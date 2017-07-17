const WebSocket = require('ws')
const path = require('path')

exports.Harness = class Harness {
  constructor () {
    this.ws = new WebSocket('ws://127.0.0.1:1983')
    this.repeatInterval = 100
  }

  get resourceDir () {
    return path.join(__dirname, 'resources')
  }

  connect () {
    return new Promise((resolve, reject) => {
      this.ws.on('error', reject)
      this.ws.on('open', () => {
        this.ws.removeListener('error', reject)
        resolve()
      })
    })
  }

  nextMessage () {
    return new Promise((resolve, reject) => {
      this.ws.removeAllListeners('message')
      this.ws.on('message', msg => {
        this.ws.removeAllListeners('message')
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

  repeatedlyCheckData (attempts, checker) {
    return this.fetchData().then(checker).catch(e => {
      if (attempts === 1) return Promise.reject(e)
      return new Promise(resolve => {
        setTimeout(() => resolve(this.repeatedlyCheckData(attempts - 1, checker)), this.repeatInterval)
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
}
