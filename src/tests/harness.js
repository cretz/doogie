const WebSocket = require('ws')
const path = require('path')
const CDP = require('chrome-remote-interface')
const util = require('util')
const robot = require('robotjs')
const assert = require('assert')

exports.Harness = class Harness {
  constructor () {
    this.ws = new WebSocket('ws://127.0.0.1:1983')
    this.repeatInterval = 100
    const self = this
    this.closeAllOnSuccess = function () {
      if (this.currentTest.state === 'failed') {
        console.log('Not closing all pages on eager test failure')
        return Promise.resolve()
      }
      return self.closeAllPages()
    }
  }

  get resourceDir () {
    return path.join(__dirname, 'resources')
  }

  inspect (obj) {
    return util.inspect(obj)
  }

  moveMouse (x, y, style = 'fastWithJitter') {
    switch (style) {
      case 'fastWithJitter':
        robot.moveMouse(x - 1, y - 1)
        robot.moveMouse(x + 1, y + 1)
        robot.moveMouse(x, y)
        break
      case 'smooth':
        robot.moveMouseSmooth(x, y)
        break
      default:
        robot.moveMouse(x, y)
    }
  }

  connect () {
    return new Promise((resolve, reject) => {
      this.ws.once('error', reject)
      this.ws.once('open', resolve)
    })
  }

  close () {
    return new Promise((resolve, reject) => {
      this.ws.once('error', reject)
      this.ws.once('close', resolve)
    })
  }

  nextMessage () {
    return new Promise((resolve, reject) => {
      this.ws.once('message', msg => {
        const data = JSON.parse(msg)
        // console.log('Data: ' + require('util').inspect(data, { depth: null }))
        if (data.error) {
          reject(data.error)
        } else {
          resolve(data)
        }
      })
    })
  }

  fetchData () {
    const p = this.nextMessage()
    this.ws.send('dump')
    return p
  }

  repeatedlyTry (attempts, func) {
    return func().catch(e => {
      if (attempts === 1) return Promise.reject(e)
      return new Promise(resolve => {
        setTimeout(() => resolve(this.repeatedlyTry(attempts - 1, func)), this.repeatInterval)
      })
    })
  }

  activate () {
    const p = this.nextMessage()
    this.ws.send('activate')
    return p
  }

  wait (ms) {
    return new Promise(resolve => setTimeout(resolve, ms))
  }

  screenshot (x, y, w, h, fileName) {
    const json = JSON.stringify({ x: x, y: y, w: w, h: h, fileName: fileName })
    const p = this.nextMessage()
    this.ws.send('screenshot' + json)
    return p
  }

  closeAllPages () {
    return this.activate()
      .then(() => robot.keyTap('f4', ['control', 'shift']))
      .then(() => this.repeatedlyTry(10, () => this.fetchData().then(data =>
        assert.equal(0, data.pageTree.items.length)
      )))
  }

  openPage (url) {
    return this.activate()
      // Open blank page
      .then(() => robot.keyTap('t', 'control'))
      .then(() => this.repeatedlyTry(10, () => this.fetchData().then(data => {
        assert.equal(1, data.pageTree.items.length)
        assert.equal(data.pageTree.items[0].text, '(New Window)')
      })))
      // Load a URL
      .then(() => {
        robot.typeString(url)
        robot.keyTap('enter')
      })
      // Make sure loading is complete
      .then(() => this.repeatedlyTry(10, () => this.treeItem().then(item =>
        assert(!item.browser.loading)
      )))
  }

  openResource (resFile) {
    return this.openPage(path.join(this.resourceDir, resFile))
  }

  treeItem (index = 0) {
    return this.fetchData().then(data => data.pageTree.items[index])
  }

  getCdp () {
    if (this.cdp) return this.cdp
    this.cdp = CDP({ host: 'localhost', port: 1989 })
    return this.cdp
  }

  closeCdp () {
    if (!this.cdp) return Promise.resolve()
    return this.getCdp().then(cdp => {
      this.cdp = null
      cdp.close()
    })
  }

  selectElement (selector, required = true) {
    return this.getCdp().then(cdp =>
      cdp.DOM.getDocument().then(doc =>
        cdp.DOM.querySelector({ nodeId: doc.root.nodeId, selector: selector }).then(elem => {
          assert(!required || elem, `Element not found for selector: ${selector}`)
          return elem
        })
      )
    )
  }

  elementBox (selector) {
    return this.getCdp().then(cdp =>
      this.selectElement(selector).then(elem => cdp.DOM.getBoxModel(elem))
    )
  }

  elementRect (selector) {
    return this.elementBox(selector).then(elemBox =>
      this.treeItem().then(item => ({
        x: item.browser.browserRect.x + elemBox.model.content[0],
        y: item.browser.browserRect.y + elemBox.model.content[1],
        w: elemBox.model.width,
        h: elemBox.model.height
      }))
    )
  }
}

exports.harness = new exports.Harness()
exports.harn = exports.harness

before(function () {
  return exports.harn.connect().then(() => exports.harn.closeAllPages())
})

after(function () {
  return exports.harn.closeCdp()
})
