KQ Lives Again!
===============

Just a tentative to continue and modernize **[KQ Lives](http://sourceforge.net/projects/kqlives/)**.

**grrk-bzzt** and **OnlineCop** are the maintainers of this project.

You can get in touch with us on IRC at **#kqfork** on ***irc.freenode.net***

How to install KQ
-----------------

First, download the latest stable version of KQ on the **[Releases](https://github.com/grrk-bzzt/kq-fork/releases)** page.

Check you have installed Allegro v4, Lua v5.2 and [DUMB](http://sourceforge.net/projects/dumb/).

Then, you can compile the project using the following commands:
```
autoreconf -i
./configure
make kq
```

Roadmap
-------

This fork aims to modernize KQ by doing the following:
* ~~Port Lua v5.1 code to Lua v5.2.~~ ***DONE!***
* Get rid of all the old DOS and BeOS dependencies.
* Port Allegro v4 code to SDL v2.
* Get rid of DUMB.
* Finish the KQ story. **You can contribute!**
* Brand new pixel art. **You can contribute!**

Known bugs
----------

* If you use PulseAudio, the game may not play any sounds. If you commit a patch for that problem, we'll gladly accept it, but in the meantime, we are just focusing on getting the game ported to SDL2.
