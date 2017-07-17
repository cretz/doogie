const assert = require('assert')
const robot = require('robotjs')
const path = require('path')

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
})
