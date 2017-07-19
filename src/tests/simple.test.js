const assert = require('assert')
const { harn } = require('./harness')

describe('Doogie', function () {
  afterEach(harn.closeAllOnSuccess)

  it('started properly', function () {
    return harn.fetchData().then(data => assert.equal(data.windowTitle, 'Doogie'))
  })

  it('opens hello world', function () {
    return harn.openResource('hello-world.html').then(() =>
      harn.repeatedlyTry(() => harn.treeItem().then(item =>
        assert.equal('Hello, World!', item.value.text)
      ))
    )
  })
})
