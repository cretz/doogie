const assert = require('assert')
const { harn } = require('./harness')

describe('Links', function () {
  afterEach(harn.closeAllOnSuccess)

  function clickLink (selector) {
    return harn.elementRect(selector).then(rect => {
      harn.moveMouse(rect.x + 5, rect.y + 5)
      harn.clickMouse()
    })
  }

  function assertInSelf () {
    return harn.repeatedlyTry(() => harn.treeItem().then(item => {
      assert(item.value.current)
      assert(item.value.items.length === 0)
      assert(item.value.text === 'Hello, World!')
    }))
  }

  function assertIsChild (background) {
    return harn.repeatedlyTry(() => harn.treeItem().then(item => {
      assert(item.value.current === background)
      assert(item.value.items.length === 1)
      assert(item.value.items[0].current !== background)
      assert(item.value.items[0].text === 'Hello, World!')
    }))
  }

  function assertForegroundTopLevel () {
    return harn.repeatedlyTry(() => harn.currentTreeItem().then(item => {
      assert(item.value.current)
      assert(item.value.items.length === 0)
      assert(item.value.text === 'Hello, World!')
    }))
    .then(() => harn.repeatedlyTry(() => harn.treeItem().then(item => {
      assert(!item.value.current)
      assert(item.value.items.length === 0)
      assert(item.value.text === 'Links')
    })))
  }

  describe('Normal link', function () {
    it('opens in itself when clicked', function () {
      return harn.openResource('links.html')
        .then(() => clickLink('#link-normal'))
        .then(() => assertInSelf())
    })

    it('opens as background child when control pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('control', () => clickLink('#link-normal')))
        .then(() => assertIsChild(true))
    })

    it('opens as foreground child when control+shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown(['control', 'shift'], () => clickLink('#link-normal')))
        .then(() => assertIsChild(false))
    })

    it('opens as foreground top-level when shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('shift', () => clickLink('#link-normal')))
        .then(() => assertForegroundTopLevel())
    })
  })

  describe('Target blank link', function () {
    it('opens as foreground child when clicked', function () {
      return harn.openResource('links.html')
        .then(() => clickLink('#link-target-blank'))
        .then(() => assertIsChild(false))
    })

    it('opens as background child when control pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('control', () => clickLink('#link-target-blank')))
        .then(() => assertIsChild(true))
    })

    it('opens as foreground child when control+shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown(['control', 'shift'], () => clickLink('#link-target-blank')))
        .then(() => assertIsChild(false))
    })

    it('opens as foreground top-level when shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('shift', () => clickLink('#link-target-blank')))
        .then(() => assertForegroundTopLevel())
    })
  })

  describe('Window.open link', function () {
    it('opens as foreground child when clicked', function () {
      return harn.openResource('links.html')
        .then(() => clickLink('#link-window-open'))
        .then(() => assertIsChild(false))
    })

    it('opens as background child when control pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('control', () => clickLink('#link-window-open')))
        .then(() => assertIsChild(true))
    })

    it('opens as foreground child when control+shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown(['control', 'shift'], () => clickLink('#link-window-open')))
        .then(() => assertIsChild(false))
    })

    it('opens as foreground top-level when shift pressed', function () {
      return harn.openResource('links.html')
        .then(() => harn.withKeyDown('shift', () => clickLink('#link-window-open')))
        .then(() => assertForegroundTopLevel())
    })
  })

  describe('Change self href', function () {
    it('changes self', function () {
      return harn.openResource('links.html?change-self')
        .then(() => assertInSelf())
    })
  })
})
