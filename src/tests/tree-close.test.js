const assert = require('assert')
const { harn } = require('./harness')

describe('Tree closing', function () {
  afterEach(harn.closeAllOnSuccess)

  function openFamily () {
    return harn.openResource('advanced-page.html?title=Granddad')
      // Make sure it's current
      .then(() => harn.treeItem().then(item => assert(item.value.current)))
      // Add a child and grand child
      .then(() => harn.openResource('advanced-page.html?title=Dad', true))
      .then(() => harn.openResource('advanced-page.html?title=Son', true))
  }

  function clickExpandOrCollapse (item, expectExpanded = undefined) {
    if (expectExpanded !== undefined) assert.equal(item.value.expanded, expectExpanded)
    harn.moveMouse(item.value.rect.x - 5, item.value.rect.y + 5)
    harn.clickMouse()
  }
  // function clickExpand (item) { clickExpandOrCollapse(item, false) }
  function clickCollapse (item) { clickExpandOrCollapse(item, true) }

  function clickClose (item) {
    harn.moveMouse(item.value.closeButton.rect.x + 5, item.value.closeButton.rect.y + 5)
    harn.clickMouse()
  }

  it('Closes children when not expanded', function () {
    // Open the family
    return openFamily()
      // Now collapse the first child
      .then(() => harn.repeatedlyTry(() => harn.treeItem('?(@.text=="Dad")').then(clickCollapse)))
      // Confirm collapsed, and then close
      .then(() => harn.repeatedlyTry(() => harn.treeItem('?(@.text=="Dad")').then(clickClose)))
      // Confirm top has no more children
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(top => assert(top.value.items.length === 0))))
  })

  it('Moves children up when expanded', function () {
    // Open the family
    return openFamily()
      // Close the first child while still expanded
      .then(() => harn.repeatedlyTry(() => harn.treeItem('?(@.text=="Dad")').then(clickClose)))
      // Confirm top has the other child
      .then(() => harn.repeatedlyTry(() => harn.treeItem().then(top =>
        assert.equal(top.value.items[0].text, 'Son')
      )))
  })
})
