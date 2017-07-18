const assert = require('assert')
const { harn } = require('./harness')

describe('Status bar', function () {
  afterEach(harn.closeAllOnSuccess)

  it('shows simple URL properly', function () {
    return harn.openResource('advanced-page.html')
      .then(() => harn.elementRect('a[href="http://example.com"]').then(rect => {
        harn.moveMouse(rect.x + 5, rect.y + 5)
      }))
      .then(() => harn.repeatedlyTry(10, () => harn.treeItem().then(item => {
        assert(item.browser.statusBar.visible)
        assert.equal(item.browser.statusBar.visibleText, 'http://example.com/')
      })))
  })

  it('adds ellipsis on long URL', function () {
    return harn.openResource('big-link-block.html')
      .then(() => harn.treeItem().then(item =>
        harn.moveMouse(item.browser.browserRect.x + 100, item.browser.browserRect.y + 100)
      ))
      .then(() => harn.repeatedlyTry(10, () => harn.treeItem().then(item => {
        assert(item.browser.statusBar.visible)
        assert(item.browser.statusBar.visibleText.endsWith('…'))
      })))
  })
})
