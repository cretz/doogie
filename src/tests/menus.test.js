const assert = require('assert')
const { harn } = require('./harness')

describe('Menus', function () {
  afterEach(harn.closeAllOnSuccess)

  const pagesMenu = 'P&ages'

  function openMenu (name) {
    return harn.activate()
      .then(data => {
        const rect = data.menu.items[name].rect
        harn.moveMouse(rect.x + 5, rect.y + 5)
        harn.clickMouse()
      })
      .then(() => harn.repeatedlyTry(() =>
        harn.fetchData().then(data => {
          assert(data.menu.items[name].visible)
          return data.menu.items[name]
        })
      ))
  }

  function execSubMenu (topMenuName, subMenuName) {
    return openMenu(topMenuName)
      .then(menuItem => {
        const rect = menuItem.items[subMenuName].rect
        harn.moveMouse(rect.x + 5, rect.y + 5)
        harn.clickMouse()
      })
  }

  describe('App menu', function () {
    it('should open top-level', function () {
      return execSubMenu(pagesMenu, 'New Top-Level Page')
        .then(() => harn.repeatedlyTry(() =>
          harn.currentTreeItem().then(item =>
            assert.equal(item.value.text, '(New Window)')
          )
        ))
    })

    it('should have child page disabled', function () {
      return openMenu(pagesMenu)
        .then(menuItem =>
          assert(!menuItem.items['New Child Foreground Page'].enabled)
        )
    })

    it('should open child page', function () {
      return harn.openResource('hello-world.html')
        .then(() => harn.treeItem())
        .then(item => {
          assert(item.value.items.length === 0)
          assert(item.value.current)
        })
        .then(() => execSubMenu(pagesMenu, 'New Child Foreground Page'))
        .then(() => harn.repeatedlyTry(() =>
          harn.treeItem().then(item => {
            assert(item.value.items.length === 1)
            assert(!item.value.current)
            assert(item.value.items[0].current)
          })
        ))
    })

    it('should only close current page', function () {
      return harn.openResource('hello-world.html')
        .then(() => harn.openResource('advanced-page.html'))
        .then(() => harn.fetchData())
        .then(data => {
          assert(data.pageTree.items[0].text === 'Hello, World!')
          assert(data.pageTree.items[1].text === 'Advanced Page')
          assert(data.pageTree.items[1].current)
        })
        .then(() => execSubMenu(pagesMenu, 'Close Current Page'))
        .then(() => harn.repeatedlyTry(() =>
          harn.fetchData().then(data => {
            assert(data.pageTree.items[0].text === 'Hello, World!')
            assert(data.pageTree.items[0].current)
          })
        ))
    })

    it('should close all pages', function () {
      return harn.openResource('hello-world.html')
        .then(() => harn.openResource('advanced-page.html'))
        .then(() => harn.fetchData())
        .then(data => {
          assert(data.pageTree.items[0].text === 'Hello, World!')
          assert(data.pageTree.items[1].text === 'Advanced Page')
          assert(data.pageTree.items[1].current)
        })
        .then(() => execSubMenu(pagesMenu, 'Close All Pages'))
        .then(() => harn.repeatedlyTry(() =>
          harn.fetchData().then(data => assert(data.pageTree.items.length === 0))
        ))
    })
  })
})
