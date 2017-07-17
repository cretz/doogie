const assert = require('assert')
const robot = require('robotjs')
const path = require('path')
const fs = require('fs')

const { Harness } = require('./harness')

const h = new Harness()

before(function () {
  return h.connect()
})

afterEach(function () {
  // Close all windows
  return h.activate().then(() => robot.keyTap('f4', ['control', 'shift']))
})

describe('doogie', function () {
  this.timeout(10000)
  it('Starts properly', function () {
    return h.fetchData().then(data => assert.equal(data.windowTitle, 'Doogie'))
  })

  it('Opens hello world properly', function () {
    return h.activate()
      // Open blank page
      .then(() => robot.keyTap('t', 'control'))
      .then(() => h.repeatedlyCheckData(10, data => {
        assert.equal(1, data.pageTree.items.length)
        assert.equal(data.pageTree.items[0].text, '(New Window)')
      }))
      // Load a URL
      .then(() => {
        robot.typeString(path.join(h.resourceDir, 'hello-world.html'))
        robot.keyTap('enter')
      })
      // Check title
      .then(() => h.repeatedlyCheckData(10, data => {
        assert.equal(data.pageTree.items[0].text, 'Hello, World!')
      }))
  })

  it('Has expected Acid3 result', function () {
    return h.activate()
      .then(() => {
        robot.keyTap('t', 'control')
        robot.typeString('http://acid3.acidtests.org/')
        robot.keyTap('enter')
        // Let's wait a bit here
        return h.wait(2000)
      })
      .then(() => h.repeatedlyCheckData(10, data => {
        // Save off the screenshot
        const actualFile = path.join(__dirname, 'temp', 'actual-acid3-screen.png')
        const expectedFile = path.join(h.resourceDir, 'expected-acid3-screen.png')
        if (!fs.existsSync(path.dirname(actualFile))) fs.mkdirSync(path.dirname(actualFile))
        const rect = data.pageTree.items[0].browser.browserRect
        return h.screenshot(rect.x, rect.y, rect.w, rect.h, actualFile).then(() => {
          // Compare the two and delete the new one only if they match
          assert.ok(fs.readFileSync(actualFile).equals(fs.readFileSync(expectedFile)))
          fs.unlinkSync(actualFile)
        })
      }))
  })
})
