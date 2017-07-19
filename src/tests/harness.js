const WebSocket = require('ws')
const path = require('path')
const CDP = require('chrome-remote-interface')
const util = require('util')
const robot = require('robotjs')
const assert = require('assert')
const http = require('http')
const fs = require('fs')
const url = require('url')

exports.Harness = class Harness {
  constructor () {
    this.repeatInterval = 100
    this.repeatAttempts = 50
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

  startResourceServer () {
    if (this.resourceServer) return Promise.reject(new Error('Already running'))
    return new Promise((resolve, reject) => {
      const resServe = http.createServer((req, res) => {
        const reqUrl = url.parse(req.url)
        res.writeHead(200)
        const stream = fs.createReadStream(
          path.join(this.resourceDir, path.normalize(reqUrl.pathname)))
        stream.once('error', e => {
          res.writeHead(500)
          res.end('Err: ' + this.inspect(e))
        })
        stream.pipe(res)
      })
      resServe.once('close', () => { this.resourceServer = null })
      resServe.listen(1993, err => {
        if (err) return reject(err)
        this.resourceServer = resServe
        resolve()
      })
    })
  }

  stopResourceServer () {
    if (!this.resourceServer) return Promise.reject(new Error('Not running'))
    return new Promise((resolve, reject) =>
      this.resourceServer.close(err => {
        if (err) {
          reject(err)
        } else {
          resolve()
        }
      })
    )
  }

  inspect (obj, opts) {
    return util.inspect(obj, opts)
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

  clickMouse (button = 'left') {
    robot.mouseClick(button)
  }

  connectWebSocket () {
    if (this.ws) return Promise.reject(new Error('Already connected'))
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket('ws://127.0.0.1:1983')
      this.ws.once('error', reject)
      this.ws.once('open', resolve)
      this.ws.once('close', () => { this.ws = null })
    })
  }

  closeWebSocket () {
    if (!this.ws) return Promise.reject(new Error('Not running'))
    return new Promise((resolve, reject) => {
      this.ws.once('error', reject)
      this.ws.once('close', resolve)
      this.ws.close()
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

  repeatedlyTry (func) {
    return this.repeatedlyTryAttempts(this.repeatAttempts, func)
  }

  repeatedlyTryAttempts (attempts, func) {
    return func().catch(e => {
      if (attempts === 1) return Promise.reject(e)
      return new Promise(resolve => {
        setTimeout(() => resolve(this.repeatedlyTryAttempts(attempts - 1, func)), this.repeatInterval)
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
      .then(() => this.repeatedlyTry(() => this.fetchData().then(data =>
        assert.equal(0, data.pageTree.items.length)
      )))
  }

  openPage (url) {
    return this.activate()
      // Open blank page
      .then(() => robot.keyTap('t', 'control'))
      .then(() => this.repeatedlyTry(() => this.fetchData().then(data => {
        assert.equal(1, data.pageTree.items.length)
        assert.equal(data.pageTree.items[0].text, '(New Window)')
      })))
      // Load a URL
      .then(() => {
        robot.typeString(url)
        robot.keyTap('enter')
      })
      // Make sure loading is complete
      .then(() => this.repeatedlyTry(() => this.treeItem().then(item =>
        assert(!item.browser.loading)
      )))
  }

  openResource (resFile) {
    return this.openPage(`http://127.0.0.1:1993/${resFile}`)
  }

  treeItem (index = 0) {
    return this.fetchData().then(data => data.pageTree.items[index])
  }

  getCdp () {
    if (this.cdp) return Promise.resolve(this.cdp)
    return CDP({ host: 'localhost', port: 1989 }).then(cdp => {
      cdp.once('disconnect', () => { this.cdp = null })
      this.cdp = cdp
      return cdp
    })
  }

  closeCdp () {
    if (!this.cdp) return Promise.resolve()
    return this.cdp.close()
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
        x: item.browser.main.rect.x + elemBox.model.content[0],
        y: item.browser.main.rect.y + elemBox.model.content[1],
        w: elemBox.model.width,
        h: elemBox.model.height
      }))
    )
  }
}

exports.harness = new exports.Harness()
exports.harn = exports.harness

// Only run this stuff if in mocha
if (global.before) {
  before(function () {
    return Promise.all([
      exports.harn.connectWebSocket().then(() => exports.harn.closeAllPages()),
      exports.harn.startResourceServer()
    ])
  })

  after(function () {
    return Promise.all([
      exports.harn.closeCdp(),
      exports.harn.closeWebSocket()
      // Too slow...
      // exports.harn.stopResourceServer()
    ])
  })
}

// If this was started via script to do something, do it
if (process.argv.length >= 3 && process.argv[2] === '--start-resource-server') {
  exports.harn.startResourceServer()
}
