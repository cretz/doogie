const assert = require('assert')
const { harn } = require('./harness')

describe('Status bar', function () {
  afterEach(harn.closeAllOnSuccess)

  it('shows simple URL properly', function () {
    return harn.openResource('advanced-page.html')
      .then(() => harn.elementRect('a[href="http://example.com"]').then(rect => {
        harn.moveMouse(rect.x + 5, rect.y + 5)
      }))
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item => {
        assert(item.value.browser.statusBar.visible)
        assert.equal(item.value.browser.statusBar.visibleText, 'http://example.com/')
      })))
  })

  it('adds ellipsis on long URL', function () {
    return harn.openResource('big-link-block.html')
      .then(() => harn.treeItem().then(item =>
        harn.moveMouse(item.value.browser.main.rect.x + 100, item.value.browser.main.rect.y + 100)
      ))
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item => {
        assert(item.value.browser.statusBar.visible)
        assert(item.value.browser.statusBar.visibleText.endsWith('…'))
      })))
  })
})
