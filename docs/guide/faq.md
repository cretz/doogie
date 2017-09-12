---
layout: guide
title: FAQ
---

# FAQ

### Why is this so buggy?

This is early alpha software and probably has a ton of bugs. This is also the original author's first foray into modern
C++, so surely there are mistakes there. They just have to be ironed out as time goes on.

### Where can I donate? How do you plan on monetizing?

There is no monetization attempt at this early stage. If you want to donate, please consider
[donating to CEF](http://www.magpcss.org/ceforum/donate.php) which this project is based on.

### Why did you choose C++ instead of a safer, more modern language?

Aside from Python, there are no mature Qt bindings and CEF bindings that would have worked well together. The original
author chooses not to use Python (or any dynamic language) for a large desktop app.

### Why do I have to sign a CLA before contributing?

Due to the nature of the project, all PRs must sign
[the CLA](https://gist.github.com/cretz/b71be6e4a65be629516405a7d21c5dd9) (digitally, prompted automatically upon first
PR). This is required to keep the project under single ownership so it may pivot in the future. Right now the license
(MIT) is really open. Users that don't want to transfer ownership or just don't want to sign the CLA for any other
reason are encouraged to maintain their own fork.