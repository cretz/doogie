const WebSocket = require('ws')
const path = require('path')
const CDP = require('chrome-remote-interface')
const util = require('util')
const robot = require('robotjs')
const assert = require('assert')
const http = require('http')
const fs = require('fs')
const url = require('url')
const jp = require('jsonpath')

exports.Harness = class Harness {
  constructor () {
    this.repeatInterval = 50
    this.repeatAttempts = 70
    const self = this
    this.closeAllOnSuccess = function () {
      if (this.currentTest.state === 'failed') {
        console.log('Not closing all pages on eager test failure')
        return Promise.resolve()
      }
      // Also press escape to get rid of any menu items open
      return self.closeAllPages().then(() => robot.keyTap('escape'))
    }
  }

  get resourceDir () {
    return path.join(__dirname, 'resources')
  }

  startResourceServer () {
    if (this.resourceServer) return Promise.reject(new Error('Already running'))
    return new Promise((resolve, reject) => {
      const resServe = http.createServer((req, res) => {
        const reqUrl = url.parse(req.url, true)

        // We'll ask for auth or check auth ("user"/"pass") if requested
        if (reqUrl.query['basicAuth']) {
          let success = false
          if (req.headers['authorization']) {
            const pieces = req.headers['authorization'].split(' ', 2)
            if (pieces.length == 2) {
              const creds = (new Buffer(pieces[1], 'base64')).toString().split(':', 2)
              console.log('Creds:', creds)
              success = creds.length == 2 && creds[0] === 'user' && creds[1] === 'pass'
            }
          }
          // No auth head means ask and leave
          if (!success) {
            res.statusCode = 401
            res.setHeader('WWW-Authenticate', 'Basic realm="Type \'user\' and \'pass\'"')
            res.end('<html><body>HTTP basic auth requested</body></html>')
            return
          }
        }

        res.writeHead(200)
        
        // We'll just return if it wants us to hang forever
        if (reqUrl.query['loadForever']) return

        // Read what was asked for
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

  // If key is an array, key is first, mods are the rest
  withKeyDown (keyAndMods, func) {
    let key = keyAndMods
    let mods = null
    if (Array.isArray(keyAndMods)) [key, ...mods] = keyAndMods
    if (mods) {
      robot.keyToggle(key, 'down', mods)
    } else {
      robot.keyToggle(key, 'down')
    }
    return Promise.resolve(func()).then(ret => {
      if (mods) {
        robot.keyToggle(key, 'up', mods)
      } else {
        robot.keyToggle(key, 'up')
      }
      return ret
    })
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

  dataQuery (expr, data = undefined) {
    const pData = data ? Promise.resolve(data) : this.fetchData()
    return pData.then(newData => jp.query(newData, expr))
  }

  dataSingleQuery (expr, data = undefined) {
    return this.dataQuery(expr, data).then(ret => ret.length > 0 ? ret[0] : null)
  }

  dataPaths (expr, data = undefined) {
    const pData = data ? Promise.resolve(data) : this.fetchData()
    return pData.then(newData => jp.paths(newData, expr))
  }

  dataPath (expr, data = undefined) {
    return this.dataPaths(expr, data).then(ret => ret.length > 0 ? ret[0] : null)
  }

  dataNodes (expr, data = undefined) {
    const pData = data ? Promise.resolve(data) : this.fetchData()
    return pData.then(newData => jp.nodes(newData, expr))
  }

  dataNode (expr, data = undefined) {
    return this.dataNodes(expr, data).then(ret => ret.length > 0 ? ret[0] : null)
  }

  repeatedlyTry (func) {
    return this.repeatedlyTryAttempts(this.repeatAttempts, func)
  }

  repeatedlyTryAttempts (attempts, func) {
    return Promise.resolve(func()).catch(e => {
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

  openPage (url, asChild = false) {
    const keyMods = asChild ? ['control', 'shift'] : 'control'
    return this.activate()
      // Get current now
      .then(() => this.currentTreeItem()).then(curr => {
        robot.keyTap('t', keyMods)
        return this.repeatedlyTry(() => this.currentTreeItem().then(newCurr => {
          if (curr) assert.notEqual(newCurr.path, curr.path)
          assert.equal(newCurr.value.text, '(New Window)')
          assert(newCurr.value.browser.urlEdit.focus)
        }))
        // Load a URL
        .then(() => {
          robot.typeString(url)
          robot.keyTap('enter')
        })
        // Make sure loading is complete
        .then(() => this.repeatedlyTry(() => this.currentTreeItem().then(newCurr => {
          assert(!newCurr.value.browser.loading)
          assert.notEqual(newCurr.value.text, '(New Window)')
        })))
      })
  }

  openResource (resFile, asChild = false) {
    return this.openPage(`http://127.0.0.1:1993/${resFile}`, asChild)
  }

  // Result is in '.value'
  treeItem (cond = '0') {
    return this.dataNode(`$.pageTree..items[${cond}]`)
  }

  currentTreeItem () {
    return this.treeItem('?(@.current==true)')
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
        x: item.value.browser.main.rect.x + elemBox.model.content[0],
        y: item.value.browser.main.rect.y + elemBox.model.content[1],
        w: elemBox.model.width,
        h: elemBox.model.height
      }))
    )
  }
}

const harness = new exports.Harness()
const harn = harness
exports.harness = harness
exports.harn = harness

// Only run this stuff if in mocha
if (global.before) {
  before(function () {
    return Promise.all([
      harn.connectWebSocket().then(() => harn.closeAllPages()),
      harn.startResourceServer()
    ])
  })

  after(function () {
    return Promise.all([
      harn.closeCdp(),
      harn.closeWebSocket()
      // Too slow...
      // harn.stopResourceServer()
    ])
  })

  // We also need some tests for ourselves
  describe('Harness', function () {
    it('Should eval JSON path properly', function () {
      const json = {
        items: [
          { current: false },
          {
            current: false,
            items: [{ current: true, name: 'Test' }]
          }
        ]
      }
      return harn.dataNode('$..items[?(@.current==true)]', json)
        .then(res => assert.equal(res.value.name, 'Test'))
    })
  })
}

// If this was started via script to do something, do it
if (process.argv.length >= 3 && process.argv[2] === '--start-resource-server') {
  exports.harn.startResourceServer()
}
