const assert = require('assert')
const { harn } = require('./harness')

describe('Browser focus', function () {
  afterEach(harn.closeAllOnSuccess)

  it('should remove URL edit focus when focused', function () {
    return harn.openResource('advanced-page.html')
      // Make sure only browser is focused
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item =>
        assert(item.value.browser.main.focus && !item.value.browser.urlEdit.focus)
      )))
      // Click the URL edit bar to switch focus
      .then(() => harn.treeItem().then(item => {
        harn.moveMouse(item.value.browser.urlEdit.rect.x + 5, item.value.browser.urlEdit.rect.y + 5)
        harn.clickMouse()
      }))
      // Make sure only the URL edit is focused
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item =>
        assert(!item.value.browser.main.focus && item.value.browser.urlEdit.focus)
      )))
      // Now click in the textarea
      .then(() => harn.elementRect('textarea').then(rect => {
        harn.moveMouse(rect.x + 5, rect.y + 5)
        harn.clickMouse()
      }))
      // Now make sure only the browser is focused
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item =>
        assert(item.value.browser.main.focus && !item.value.browser.urlEdit.focus)
      )))
  })
})
