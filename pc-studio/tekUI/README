
== Table of contents ==

 - 1. [[#about : About tekUI]]
  - 1.1. [[#license : License]]
  - 1.2. [[#status : Status, requirements]]
  - 1.3. [[#support : Support and services]]
  - 1.4. [[#authors : Authors, links, downloads]]

 - 2. [[#overview : Technical overview]]
  - 2.1. [[#features : General features]]
  - 2.2. [[#uielements : Supported user interface elements]]
  - 2.3. [[#deployment : Deployment]]
  - 2.4. [[#themes : Style sheets and themes]]
  - 2.5. [[#clibrary : C library]]
  - 2.6. [[#objectmodel : Lua, C and object model]]
  - 2.7. [[#xmlapps : Writing applications in XML]]
  - 2.8. [[#documentation : Documentation system]]
  - 2.9. [[#embedded : Fitness for the use in embedded systems]]
  - 2.10. [[#multitasking : Multitasking interface]]
  - 2.11. [[#futureplans : Future plans]]

 - 3. [[#buildinstall : Building and installing]]
  - 3.1. [[#requirements : Requirements]]
   - 3.1.1 [[#luamodules : External Lua modules]]
  - 3.2. [[#buildenv : Adjusting the build environment]]
   - 3.2.1 [[#notelinux : Linux notes]]
   - 3.2.2 [[#notebsd : BSD notes]]
   - 3.2.3 [[#notewindows : Windows notes]]
   - 3.2.4 [[#notehpux : HP-UX notes]]
   - 3.2.5 [[#notex11 : X11 notes]]
   - 3.2.6 [[#noterawfb : Raw framebuffer notes]]
   - 3.2.7 [[#notedirectfb : DirectFB notes]]
   - 3.2.8 [[#notefloat : Floating point support]]
   - 3.2.9 [[#noteluajit : LuaJIT support]]
  - 3.3. [[#building : Building]]
  - 3.4. [[#installation : Installation]]
  - 3.5. [[#envvariables : Environment variables]]
  - 3.6. [[#extraargs : Global Lua runtime arguments]]

 - 4. [[#usersguide : User's Guide]]
  - 4.1. [[#helloworld : Hello, World!]]
  - 4.2. [[#reactingoninput : Reacting on input]]
  - 4.3. [[#layout : Controlling the layout]]
  - 4.4. [[#handlers : List of predefined handlers]]
  - 4.5. [[#adhocclasses : Ad-hoc setup of classes]]

 - 5. [[#developersguide : Developer's Guide]]
  - 5.1. [[#lifecycle : The lifecycle of an element]]
  - 5.2. [[#debug : Debug library]]
  - 5.3. [[#proxied : Debugging objects]]
  - 5.4. [[#classsetup : Class setup]]
  - 5.5. [[#classdoc : Class documentation system]]
  - 5.6. [[#cssprecedence : Stylesheets precedence]]

=== Links ===

 * [[tek.ui][manual.html#tek.ui]] module - library entrypoint
 * [[manual.html : Complete class and library reference manual]]
 * [[changes.html : Changelog]]

---------------------------------------------------------------------

=( about : 1. About tekUI )=

TekUI is a small, freestanding and portable graphical user interface
(GUI) toolkit written in Lua and C. It was initially developed for
the X Window System and has been ported to Linux framebuffer, raw
framebuffer, DirectFB, Windows, and other displays since. A
[[VNC server option][#noterawfb]] is available that allows the remote
control of tekUI applications.

Its main focus is the rapid development of custom-made applications
with a custom appearance, such as for embedded devices with a display
controller 
(see also [[#embedded : fitness for the use in embedded systems]]).
In the long term, it is intended to feature a class library
supporting regular, general-purpose desktop and mobile applications.

Being mostly written in the
[[Lua scripting language][http://www.lua.org/]] and using a simple
inheritance scheme and class library, tekUI is easily extensible with
new user interface elements. New controls can be written (or
prototyped) in Lua and ported to C later. The creation of new styles
and themes and fitting the software to a new device are equally
simple.

See below for a more detailed technical [[#overview]].

==( license : 1.1. License )==

TekUI is free software under the same license as Lua itself: It can
be used for both academic and commercial purposes at no cost. It
qualifies as Open Source software, and its license is compatible with
the GPL – see [[copyright][copyright.html]]. Additional components
are available under a commercial license – see below.

==( status : 1.2. Status, requirements )==

TekUI is slowly approaching feature completeness ("beta" stadium). It
is available for Windows and Unix-like operating systems such as
Linux, BSD, Mac OS X, Solaris and HP-UX, and it may work on other
flavours of Unix with few modifications.

Display drivers are included for X11, Windows, Linux framebuffer,
raw memory, and DirectFB. See also [[TODO][todo.html]] for a list of
known bugs and missing features, and [[#requirements]] for a list of
the required libraries.

==( support : 1.3. Support and services )==

Special adaptations, custom display drivers, new user interface
elements, and support and services for the integration of tekUI into
your products are available from the authors – see below. A
commercially licensed and supported version is available on request.

==( authors : 1.4. Authors, links, downloads )==

Authors and contributors:
 - Timm S. Müller <tmueller at schulze-mueller.de>
 - Franciska Schulze <fschulze at schulze-mueller.de>
 - Tobias Schwinger <tschwinger at isonews2.com>

Open source project website:
 - http://tekui.neoscientists.org/

Release downloads:
 - http://tekui.neoscientists.org/releases/

Mailing list:
 - http://lists.neoscientists.org/mailman/listinfo/tekui-devel

Product website:
 - http://www.schulze-mueller.de/products.html

---------------------------------------------------------------------

=( overview : 2. Technical overview )=

TekUI is not a binding to an existing GUI library, and it makes no
attempt to conform to the looks of a host's native GUI (although this
can be emulated to a certain degree – see below for details). The
implementation is comparatively fast and resource-friendly, despite
its script-heavy nature. Among the benefits for developers are its
simplicity, transparency and modularity; in particular, custom
interface elements can be created with little effort.

==( features : 2.1. General features )==

 * Strictly event-driven
 * Automatic layouting and scalability
 * Font-sensitivity, support for antialiased fonts
 * UTF-8 and localization support, Unicode/metadata strings
 * Support for cascading style sheets (CSS) and themes
 * Allows for multi-windowed, tabbed and fullscreen applications
 * Incremental refresh logic with minimal overdraw
 * Concurrency thanks to inbuilt support for dispatching to
 coroutines, preemptive tasks also supported
 * Applications can be written in Lua code, nested expressions, or
 XML. The XML launcher can validate against the actual implementation
 * Works unmodified with stock (double precision), single precision
 and integer versions of the Lua VM
 * Supplied with a documentation system supporting tekUI's object
 model
 * Support for textures, color gradients and transparency
 * A text editor is included as an example
 * A graphical Lua compiler/linker/amalgamation tool is included
 
==( uielements : 2.2. Supported interface elements )==

 * Canvas
 * Checkmark
 * Directory lister
 * File requester
 * Floating text
 * Gauge
 * Group (horizontal, vertical, grids, in pages, scrollable)
 * Handle for group balancing
 * Images
 * Lister, also multi-column and multi-selection
 * Menu (popup and window), also nested
 * Popup item, also nested
 * Popup list ('combo box')
 * Radiobutton
 * Scrollgroup
 * Slider
 * Spacer
 * Text button (and label), also multi-line
 * Text input (single and multi-line, passwords, scrolling, ...)

==( deployment : 2.3. Deployment )==

Languages and formats currently supported for writing applications in
are Lua, and to some lesser extent, XML. Applications can communicate
with their host using

 * C bindings
 * I/O streams (see meter example)
 * datagram sockets (see socketmessage example, using LuaSocket)
 * pipes between parent and child process (see childprocess example,
 using luaposix)
 * inter-task messages and signals (see childtask and childnotify
 examples)
 
See also [[#futureplans : Future plans]] and 
[[#xmlapps : Writing applications in XML]].

==( themes : 2.4. Style sheets and themes )==

TekUI comes with a cascading style sheets (CSS) engine for the
declaration of styles for element classes, custom classes, pseudo
classes, as well as for individual, direct and hardcoded class
formattings. 

The only admission to a more common appearance is that tekUI tries to
import the color scheme found in a GTK+ configuration file, if the
{{"desktop"}} style sheet is used (which is the default, see also
[[environment variables][#envvariables]]). If you are using KDE and
check the ''Apply colors to non-KDE applications'' option, KDE, GTK+
and tekUI all share the same color scheme.

==( clibrary : 2.5. C library )==

The C library is based on the [[TEKlib][http://teklib.neoscientists.org/]]
middleware project. All required modules are contained in tekUI's
source code distribution, to reduce the hassle of building and
installing the software.

The C library isolates from the host and provides
performance-critical routines (e.g. the region management, default
layouter and string library). Rendering and input handling are
implemented as display drivers, which allow for easy exchangeability.

Aside from the display driver interface, the C library implements
OS-like facilities such as portable threads, a timer device and can
support a virtual filesystem interface.

==( objectmodel : 2.6. Lua, C and object model )==

Regardless of whether classes are written in Lua or C, they share a
common, single-inheritance object model, which is determined by the
Lua virtual machine and provides a referencing scheme and automatic
memory management.

Classes can be written in Lua and C interchangeably, meaning that
they can inherit from each other regardless of the language they are
written in. This makes it possible to prototype applications
quickly, and, should the need arise, to replace
performance-crititcal classes with their counterparts in C. When
porting classes to C, there is a certain degree of freedom in how
much reliance on the Lua VM and its automatic resource management
is desired.

==( xmlapps : 2.7. Writing applications in XML )==

TekUI applications can be written as nested expressions in Lua, and
this makes it easy to convert hierarchically structured formats such
as XML to fully functional applications that can be executed on the
Lua virtual machine. An examplary XML runner is included, e.g.:

  # bin/runxml.lua -e tutorial-5.xml

One benefit of writing an application in XML is that some validity
checks can be performed against a document type, and also against
tekUI's actual implementation:

  # bin/runxml.lua -c tutorial-5.xml

For this, it is required that you are using a debug version of
tekUI's base class, see [[#proxied : Debugging objects]].

XML support is still somewhat experimental. See the accompanying
examples on how to express interactions between elements using
notifications and the embedding of methods.

==( documentation : 2.8. Documentation system )==

TekUI comes with a documentation generator supporting its own object
model. It is capable of generating a function reference and
hierarchical class index from specially crafted comments in the
source code. To regenerate the full documentation, invoke

  # make docs

Note that you need
[[LuaFileSystem][http://www.keplerproject.org/luafilesystem/]] for
the document generator to process the file system hierarchy.

==( embedded : 2.9. Fitness for the use in embedded systems )==

TekUI is as embeddable as Lua itself. Global data is basically
absent; there are a few extras requiring them, {{ENABLE_XSHM}}
and {{ENABLE_LAZY_SINGLETON}}, that can be disabled or reworked.
Resting on an OS-like abstraction, it can be taken to environments
with no operating system at all, if just memory and a minimal
threading API is present (create, wait, signal, and e.g. atomic
in-/decrement). A display driver can be written on that premise and
tekUI's C library alone.

Complete solutions based on tekUI are in the reach of 512KiB ROM or
flash memory: A typical application and the required modules, if
compiled to Lua bytecode, can fit into 256KiB. Add to that approx.
128KiB for the C modules and another 128KiB for the Lua virtual
machine.

Generic, unmodified versions of tekUI have been deployed on PPC, ARM,
x86, and MIPS based microcontrollers with clock rates as low as
200MHz, while maintaining good reactivity and a pleasant look & feel.
Lower clock rates are workable with some adaptations, by disabling
expensive features like style sheet support, by porting certain
classes to C, using display drivers which are more tightly connected
to the hardware, etc.

==( multitasking : 2.10. LuaExec multitasking interface )==

TekUI rests on a C library that can provide preemptive multitasking
not only to GUI programs, but to all kinds of Lua applications. It
is fully contained in the tekUI package, but it can be used
independently and is also available as a separate project,
[[LuaExec][http://luaexec.neoscientists.org/]], which also provides
a [[tutorial and primer][http://luaexec.neoscientists.org/#tutorial]]
on multitasking.

==( futureplans : 2.11. Future plans )===

Since version 1.12 tekUI supports messaging and signals between
preemptive tasks. While these are already available to Lua programs,
future versions shall additionally provide a C library interface for
the same purpose. See also above on tekUI's multitasking interface.

One future development goal is to fold the tekUI framework into
a freestanding C library, which enables applications to create
asynchronous GUI objects, communicating with their main program
using an application-level protocol. From this, the following
benefits are envisioned:

 * GUIs integrate smoothly even into I/O-burdened applications
 (clients, servers)
 * strict separation of GUI and functionality; the GUI runs in
 a thread or process of its own
 * Faulty GUI application code, as it is written in a safe language,
 cannot corrupt the heap or otherwise crash the device, even if it
 is without a MMU.

Lua would continue to act as the toolkit's internal scripting
language, which can be used for complex interconnections
between GUI elements as well as many application tasks.

---------------------------------------------------------------------

=( buildinstall : 3. Building and installing )=

==( requirements : 3.1. Requirements )==

Development tools and libraries alongside with their tested versions
(as of this writing) are given below:

 * Lua 5.1, 5.2 or 5.3
 * >=libX11-1.1.3 (for the x11 driver)
  * >=libXft-2.1.12 (for the x11 driver, enabled by default)
  * >=fontconfig-2.5.0 (for the x11 driver, enabled by default)
  * XFree86VidMode extension (for the x11 driver, disabled by default)
 * >=freetype-2.3.5 (for the rawfb and directfb drivers)
 * >=DirectFB-0.9.25.1 (for the directfb driver)
 * LibVNCServer-0.9.9 (for the rawfb driver, disabled by default)
 * libpng (for PNG support, disabled by default)
 * MinGW (for building on the Windows platform)

Regarding LuaJIT 2.0 see annotations below.

===( luamodules : 3.1.1. External Lua modules )===

TekUI does not strictly depend on any external Lua modules. The
following modules are supported:

 * [[LuaFileSystem][http://www.keplerproject.org/luafilesystem/]]
 for the [[DirList][manual.html#tek.ui.class.dirlist]] class,
 [[#documentation]] generator, and main demo program
 * [[LuaExpat][http://matthewwild.co.uk/projects/luaexpat/]] for
 the XML runner
 * [[LuaSocket][http://w3.impa.br/~diego/software/luasocket/]]
 for the socketmessage example
 * [[luaposix][https://github.com/luaposix/luaposix]] for the
 childprocess example
 
==( buildenv : 3.2. Adjusting the build environment )==

TekUI was at some point tested and should compile and run on:

 * Ubuntu Linux (tested: 14.04, 9.10, 8.04, 7.10)
 * FreeBSD (tested: 10.1, 8.1, 7.0), NetBSD (tested: 6.0), special
 precautions needed, see [[BSD notes][#notebsd]]
 * Gentoo Linux (tested: amd64/13.0, x86/10.0, x86/2008.0,
 amd64/2007.0, ppc/2007.0)
 * Windows (tested: 7, 2000, Wine)
 * Mac OS X (tested: 10.10, XQuartz)
 * Solaris (tested: 11, Sparc, x86)
 * HP-UX (tested: 11.31, HPPA, IA64), special precautions needed,
 see [[HP-UX notes][#notehpux]]

If building fails for you, you may have to adjust the build
environment, which is located in the {{config}} file on the topmost
directory level. Supported build tools are {{gmake}} (common under
Linux) and {{pmake}} (common under FreeBSD).

===( notelinux : 3.2.1. Linux notes )===

By popular request, these are the names of packages required to
compile and run tekUI on Ubuntu Linux: {{lua5.1}},
{{liblua5.1-0-dev}}, {{liblua5.1-filesystem0}}, {{libfreetype6-dev}},
{{libxft-dev}}, {{libxext-dev}}, {{libxxf86vm-dev}}.

===( notebsd : 3.2.2. BSD notes )===

On FreeBSD and NetBSD you need a Lua binary which is linked with the
{{-pthread}} option, as tekUI is using multithreaded code in shared
libraries, which are dynamically loaded by the interpreter. For
linking on NetBSD uncomment the respective section in the {{config}}
file. On recent versions of FreeBSD, you will possibly want to change
the compiler to {{clang}}.

===( notewindows : 3.2.3. Windows notes )===

The native Windows display driver was incomplete, slow and shabbily
supported for a long time, but we have received a number of
high-class contributions and spent some additional effort on this
driver. It is functionally on par now and runs crisply.

For adjusting the build environment, see the {{config}} file. We can
recommend using the MSYS shell and MinGW under Windows, or to
cross-build for Windows using GCC or clang. An example configuration
is provided in {{etc/}}.

===( notehpux : 3.2.4. HP-UX notes )===

In addition to the HP-UX settings and notes in the {{config}} file,
you need a Lua binary with a patch for loading shared libraries via
{{shl_load()}}, and pthreads support. For building Lua, use 

  CPPFLAGS=-D_POSIX_C_SOURCE=199506L
  LDFLAGS=-lpthread

===( notex11 : 3.2.5. X11 notes )===

By default, libXft and fontconfig are needed for building the x11
driver. But libXft is unnecessary if you stack the rawfb driver on
top of the x11 driver. Disable it by removing
{{-DENABLE_XFT}} from {{X11_DEFS}}. 

The XFree86VidMode extension is no longer linked against by default.
It can be enabled by adding {{-DENABLE_XVID}} to {{X11_DEFS}} in the
{{config}} file. X11 fullscreen support is problematic.

===( noterawfb : 3.2.6. Raw framebuffer notes )===

The raw framebuffer driver performs all rendering in a chunk of
memory that can be supplied by a native display or the application.

It is recursive in that another tekUI display driver can be plugged
into it as a visualization and input device. This was originally
intended to act as a template (or skeleton) for specializations in
a target context, like custom hardware or a 3D game or simulation,
but it also comes in useful for providing a backbuffer for other
display types.

Support for the native Linux framebuffer (and input event interface)
can be compiled into the raw framebuffer directly. It then adapts to
the active screensize and pixel format automatically. The keymap is
compiled in, by default {{/usr/share/keymaps/i386/qwerty/us.map.gz}},
and can be generated using a conversion tool, {{etc/keymap2c}}.

A VNC server option (through 
[[LibVNCServer][http://libvncserver.sourceforge.net/]]) is available
for this driver, which allows a tekUI application to be shared over
the network. This works both "headless" and with a local display. To
enable LibVNCServer support, enable the raw framebuffer and the
{{VNCSERVER_}} defines in the config file. Note: VNC support makes
this software a combined work with LibVNCServer, see also
{{src/display_rawfb/vnc/COPYING}}.

Any combination of the subdrivers tekUI, Linux framebuffer, and VNC
is valid. If none is selected, the GUI runs isolated from the outside
world, albeit in a very portable way, in its own chunk of memory.

The default configuration assumes the free fonts ''DejaVuSans'' and
''DejaVuSansMono'' (in combination with their subtypes ''Bold'' and
''Oblique'') in the local directoy {{tek/ui/font}}, but you can also
point the [[environment variable][#envvariables]] {{FONTDIR}} to a
directory containing these fonts. If they are unavailable, tekUI
performs various fallbacks, including to Bitstream ''Vera'',
which is small enough to be included in tekUI's distribution.

A mouse pointer image is loaded from {{DEF_CURSORFILE}}, by default
{{tek/ui/cursor/cursor-black.png}}, with a fallback to a small,
built-in pointer image.

===( notedirectfb : 3.2.7. DirectFB notes )===

The DirectFB display driver is not very actively supported at this
time. Some adaptations are likely needed. In case of problems,
consider using the raw framebuffer driver, or the DirectFB driver
as a rendering back-end for the raw framebuffer driver.

===( notefloat : 3.2.8 Floating point support )===

TekUI is free of floating point arithmetics and works with a 32bit
integer version of the Lua virtual machine, with the following,
noncritical exceptions:

 * Support for color gradients (see {{-DENABLE_GRADIENT}} in the
 config file) requires the {{float}} data type in the C library. 
 * The desktop style needs floating point support in the Lua VM to
 import GTK+ colors (falls back to the default colorscheme if
 unavailable at runtime).

===( noteluajit : 3.2.9 LuaJIT support )===

TekUI works with LuaJIT 2.0 - as long as tekUI's
[[#multitasking : multitasking interface]] is not used,
and possibly even then, provided that LuaJIT implements or can be
made to implement {{lua_newstate()}} without reporting an error.
On x86_64 this has been found to not be the case. Currently the
impact of not using tekUI's multitasking API is small, and tekUI
should correctly handle the case by reporting that it could not
create a child task on LuaJIT.

==( building : 3.3. Building )==

To see all build targets, type

  # make help

The regular build procedure is invoked with

  # make all

==== Building a tekUI executable ====

To build a standalone executable implementing a Lua 5.1/5.2/5.3
interpreter, tekUI's C libraries and parts of its class library,
adjust {{LUADISTDIR}}, {{LUAEXE_DEFS}} and {{LUAEXE_LIBS}} in the
{{config}} file, and invoke

  # make tools

The resulting executable can then be found in {{bin/tekui.exe}}.
If this executable is present, the {{bin/compiler.lua}} tool will
additionally offer to save Lua programs into standalone executables.

The exact classes to be built into the executable can be adjusted
in {{tek/lib/MODLIST}}. This list will also be taken into account
by {{bin/compiler.lua}} to avoid linking modules twice.

==( installation : 3.4. Installation )==

A system-wide installation of tekUI is not strictly required – in so
far as the majority of display drivers is concerned. The DirectFB
driver, in contrast, looks up fonts and cursors globally and must be
installed in any case.

Once tekUI is built, it can be worked with and developed against, as
long as you stay in the top-level directory of the distribution; all
required modules and classes will be found if programs are started
from there, e.g.:

  # bin/demo.lua

If staying in the top-level directory is not desirable, then tekUI
must be installed globally. By default, the installation paths are

 - {{/usr/local/lib/lua/5.1}}
 - {{/usr/local/share/lua/5.1}}

It is not unlikely that this is different from what is common for
your operating system, distribution or development needs, so be sure
to adjust these paths in the {{config}} file. The installation is
conveniently invoked with

  # sudo make install

==( envvariables : 3.5. Environment variables )==

 * {{FONTDIR}} - TTF font directory for the rawfb and directfb
 drivers, overriding the hardcoded default ({{tek/ui/font}})
 * {{FULLSCREEN}} - {{"true"}} to try opening an application in
 fullscreen mode. Note that for this option to take effect, the
 application's first window must have the {{RootWindow}} attribute
 set as well.
 * {{GTK2_RC_FILES}} - Possible locations of a gtkrc-2.0
 configuration file.
 * {{NOCURSOR}} - {{"true"}} to disable the mouse pointer over a
 tekUI application. This may be useful when running on a touch
 screen.
 * {{THEME}} - Theme names corresponding to style sheet files in
 {{tek/ui/style}}. Multiple themes can be separated by spaces,
 e.g. {{"desktop gradient"}}. The default is {{"desktop"}}. The
 ''desktop'' theme tries to import the color scheme found in a GTK2
 configuration file. See also [[CSS precedence][#cssprecedence]].
 * {{VNC_PORTNUMBER}} - Specifies a VNC server's port number.

==( extraargs : 3.6. Global Lua runtime arguments )==

Some properties of tekUI are configurable globally from Lua,
by modifying fields in the {{ui}} library at program start:

 * {{ui.ExtraArgs}} - mostly used for hints to display drivers,
 one argument per line, currently supported:
  * {{fb_backbuffer=boolean}} - If enabled, the framebuffer driver
  uses a backbuffer even if not stricly needed. This can reduce
  flicker and sometimes improve performance, although this varies
  widely. Raspberry Pi, for example, was found to perform best on
  16bit without backbuffer.
  * {{fb_pixfmt=hexnumber}} - Specify desired pixel format for the
  framebuffer, e.g. {{5650120b}}. See {{include/tek/mod/visual.h}}
  for the defines prefixed with {{TVPIXFMT_}}.
  * {{fb_winbackbuffer=boolean}} - If enabled, windows on a
  framebuffer use their own backing-store and do not encumber the
  application with refreshes when moved or otherwise exposed.
  * {{vnc_portnumber=num}} - Overrides {{VNC_PORTNUMBER}}, see above
 * {{ui.FullScreen}} - boolean, overrides {{FULLSCREEN}} (see above)
 * {{ui.Mode}} - set this to {{"workbench"}} for windows to receive
 buttons for dragging, resizing, closing, etc. This is intended for
 running tekUI in a desktop-like manner on display subsystems which
 do not provide a window manager.
 * {{ui.MsgFileNo}} - Number of the file descriptor used for 
 dispatching lines to messages of type {{MSG_USER}}. Default:
 standard input. See also the ''childprocess'' example.
 * {{ui.NoCursor}} - Overrides {{NOCURSOR}}, see above
 * {{ui.ShortcutMark}} - Character to indicate that the following
 character is used for a keyboard shortcut. Default: {{"_"}}
 * {{ui.ThemeName}} - overrides {{THEME}}, see above. See also
 [[CSS precedence][#cssprecedence]].
 * {{ui.UserStyles}} - see [[CSS precedence][#cssprecedence]]

---------------------------------------------------------------------

=( usersguide : 4. User's Guide )=

==( helloworld : 4.1. Hello, World! )==

The GUI version of the 'Hello, World!' program:

  ui = require "tek.ui"
  ui.Application:new
  {
    Children =
    {
      ui.Window:new
      {
        Title = "Hello",
        Children =
        {
          ui.Text:new
          {
            Text = "Hello, World!",
            Class = "button",
            Mode = "button",
            Width = "auto"
          }
        }
      }
    }
  }:run()

As can be seen, tekUI allows a fully functional application to be
written in a single nested expression. The
[[manual.html#tek.ui : UI]] library comes with an on-demand class
loader, so whenever a class (like
[[manual.html#tek.ui.class.application : Application]],
[[manual.html#tek.ui.class.window : Window]] or
[[manual.html#tek.ui.class.text : Text]]) is accessed for the first
time, it will be loaded from {{tek/ui/class/}} in the file system.

Note that a button class is not strictly needed: a button is just a
Text element behaving like a button with a frame giving it the
appearance of a button. We will later explain how you can write a
button class yourself, to save you some typing.

To quit, click the window's close button. Closing the
[[manual.html#tek.ui.class.application : Application]]'s last
open window will cause the {{run}} method to return to its caller.

==( reactingoninput : 4.2. Reacting on input )==

There are different ways for reacting to presses on the 'Hello,
World' button. The simplest form is to place an 
[[Object:onClick][manual.html#Object:onClick]] function into the
Text object:

  ui = require "tek.ui"
  ui.Application:new
  {
    Children =
    {
      ui.Window:new
      {
        Title = "Hello",
        Children =
        {
          ui.Button:new
          {
            Text = "Hello, World!",
            Width = "auto",
            onClick = function(self)
              print "Hello, World!"
            end
          }
        }
      }
    }
  }:run()

But {{onClick}} is just a convenient shortcut for the most trivial
of all cases. To catch more events, you can override handler
functions which react on changes to an element's state variables,
like {{Pressed}} and {{Selected}}, to name but a few. They contain
booleans and are indicative of the Element's state, such as: Is it in
selected state, is it pressed down, is it hovered by the mouse, is it
receiving the input? The 
[[Widget:onPress][manual.html#Widget:onPress]] handler, for example,
can be used to catch not only releases, but also presses on the
element:

  ui = require "tek.ui"
  ui.Application:new
  {
    Children =
    {
      ui.Window:new
      {
        Children =
        {
          ui.Text:new { },
          ui.Button:new
          {
            Text = "Click",
            Width = "auto",
            onPress = function(self)
              ui.Button.onPress(self)
              self:getPrev():setValue("Text", tostring(self.Pressed))
            end
          }
        }
      }
    }
  }:run()

When you overwrite a handler, you should forward the call to the
original implementation of the same method, as seen in the example.

Setting a value using 
[[Object:setValue][manual.html#Object:setValue]] may invoke a
notification handler. In our example, the neighboring element in the
group will be notified of an updated text, which will cause it to be
repainted.

For regular applications it is normally sufficient to stick to
overwriting the available [[#handlers]] as in the previous example.
But the underlying mechanism to register a notification handler can
be interesting as well, especially if you plan on writing new GUI
classes yourself:

  ui = require "tek.ui"
  app = ui.Application:new()
  win = ui.Window:new { Title = "Hello", HideOnEscape = true }
  text = ui.Button:new { Text = "_Hello, World!", Width = "auto" }
  text:addNotify("Pressed", false, {
    ui.NOTIFY_SELF,
    ui.NOTIFY_FUNCTION,
    function(self)
      print "Hello, World!"
    end
  })
  win:addMember(text)
  app:addMember(win)
  app:run()

See also [[Object:addNotify][manual.html#Object:addNotify]] for
all the hairy details on notification handlers, and the
[[manual.html#tek.ui.class.area : Area]] and
[[manual.html#tek.ui.class.widget : Widget]] classes for the most
important state variables.

==( layout : 4.3. Controlling the layout )==

By default a tekUI layout is dynamic. This means it will be
calculated and, if necessary, recalculated at runtime¹. Screen and
font characteristics are taken into account as well as style
properties controlling the appearance of individual elements.

==== Group layouting attributes ====

 * {{Orientation}} - {{"horizontal"}} or {{"vertical"}}, the default
 is {{"horizontal"}}. This attribute informs the group of the primary
 axis on which elements are to be layouted (left-to-right vs.
 top-to-bottom). This applies to grids also.
 
 * {{Columns}} - Number of columns. A number greater than {{1}}
 turns the group into a grid.
 
 * {{Rows}} - Number of rows. A number greater than {{1}} turns the
 group into a grid.
 
 * {{Layout}} - The name of a layouting class. Default: {{"default"}},
 which will try to load the {{tek.ui.layout.default}} class. It is
 also possible to specify an instance of a layouter instead of just
 the class name.
 
 * {{SameSize}} - boolean, {{"width"}} or {{"height"}}. Default is
 '''false'''. If true, all elements in the group will have the same
 size on their respective axis. If either of the string keywords is
 given, this applies to only to the width or height.

==== Common layouting attributes (apply to all elements) ====
  
 * {{HAlign}} - The element's horizontal alignment inside the group
 which it is part of. Possible values are {{"left"}}, {{"center"}},
 and {{"right"}}. The corresponding style property is ''halign''.
 
 * {{VAlign}} - The element's vertical alignment inside the group
 which it is part of. Possible values are {{"top"}}, {{"center"}},
 and {{"bottom"}}. The corresponding style property is ''valign''.
 
 * {{Width}} - The width of the element, in pixels, or {{"auto"}}
 to reserve the minimum size, {{"free"}} for allowing the element
 to grow to any size, or {{"fill"}} for allowing the element to
 grow to no more than the maximum width that other elements in the
 same group have claimed. The corresponding style property is
 ''width''.
 
 * {{Height}} - The height of the element, in pixels, or {{"auto"}}
 to reserve the minimum size, {{"free"}} for allowing the element
 to grow to any size, or {{"fill"}} for allowing the element to
 grow to no more than the maximum height that other elements in the
 same group have claimed. The corresponding style property is
 ''Height''.

 * {{MinWidth}} - The minimum width of the element, in pixels. The
 default is {{0}}. The corresponding style property is ''min-width''.
 
 * {{MinHeight}} - The minimum height of the element, in pixels. The
 default is {{0}}. The corresponding style property is
 ''min-height''.
 
 * {{MaxWidth}} - The maximum width of the element, in pixels, or
 {{"none"}} for no limit (which is the default). The corresponding
 style property is ''max-width''.
 
 * {{MaxHeight}} - The maximum height of the element, in pixels, or
 {{"none"}} for no limit (which is the default). The corresponding
 style property is ''max-height''.

Note that the {{Min}}/{{Max}} and {{Width}}/{{Height}} properties
will not override the actual size requirements of an element. An
element will not claim a larger or smaller size than what it is
capable of displaying. Style properties will be used as additional
hints when an element's size is flexible. As most elements are
scalable by nature, the style properties are normally considered.

---------------------------------------------------------------------
 
¹ More layouting options are available, see also the {{fixed.lua}}
and {{layouthook.lua}} examples on how to use and implement fixed
and custom layouting strategies.

==( handlers : 4.4. List of predefined handlers )==

'''Name'''                || '''Base Class'''                                     || '''Cause'''
Widget:onActivate()       || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Active}} attribute 
Window:onChangeStatus()   || [[manual.html#tek.ui.class.window : Window]]         || change of {{Status}} attribute
Widget:onClick()          || [[manual.html#tek.ui.class.widget : Widget]]         || caused when {{Pressed}} changes to '''false'''
Widget:onDisable()        || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Disabled}} attribute
Widget:onDblClick()       || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{DblClick}} attribute
Input:onEnter()           || [[manual.html#tek.ui.class.input : Input]]           || change of {{Enter}} attribute, pressing enter
Widget:onFocus()          || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Focus}} attribute
Window:onHide()           || [[manual.html#tek.ui.class.window : Window]]         || window close button, Escape key
Widget:onHilite()         || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Hilite}} attribute
Widget:onHold()           || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Hold}} attribute
Widget:onPress()          || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Pressed}} attribute
Widget:onSelect()         || [[manual.html#tek.ui.class.widget : Widget]]         || change of the {{Selected}} attribute
DirList:onSelectEntry()   || [[manual.html#tek.ui.class.dirlist : DirList]]       || item selected by the user
Lister:onSelectLine()     || [[manual.html#tek.ui.class.lister : Lister]]         || change of the {{SelectedLine}} attribute
PopList:onSelectLine()    || [[manual.html#tek.ui.class.poplist : PopList]]       || change of the {{SelectedLine}} attribute
Input:onSetChanged()      || [[manual.html#tek.ui.class.input : Input]]           || setting the {{Changed}} attribute, on text changes
Canvas:onSetChild()       || [[manual.html#tek.ui.class.canvas : Canvas]]         || change of the {{Child}} attribute
Element:onSetClass()      || [[manual.html#tek.ui.class.element : Element]]       || change of the {{Class}} attribute
Lister:onSetCursor()      || [[manual.html#tek.ui.class.lister : Lister]]         || change of the {{CursorLine}} attribute
ImageWidget:onSetImage()  || [[manual.html#tek.ui.class.imagewidget : ImageWidget]] || change of the {{Image}} attribute
Numeric:onSetMax()        || [[manual.html#tek.ui.class.numeric : Numeric]]       || change of the {{Max}} attribute
ScrollBar:onSetMax()      || [[manual.html#tek.ui.class.scrollbar : ScrollBar]]   || change of the {{Max}} attribute
Numeric:onSetMin()        || [[manual.html#tek.ui.class.numeric : Numeric]]       || change of the {{Min}} attribute
ScrollBar:onSetMin()      || [[manual.html#tek.ui.class.scrollbar : ScrollBar]]   || change of the {{Min}} attribute
PageGroup:onSetPageNumber() || [[manual.html#tek.ui.class.pagegroup : PageGroup]] || change of the {{PageNumber}} attribute
ScrollBar:onSetRange()    || [[manual.html#tek.ui.class.scrollbar : ScrollBar]]   || change of the {{Range}} attribute
Slider:onSetRange()       || [[manual.html#tek.ui.class.slider : Slider]]         || change of the {{Range}} attribute
Element:onSetStyle()      || [[manual.html#tek.ui.class.element : Element]]       || change of the {{Style}} attribute
FloatText:onSetText()     || [[manual.html#tek.ui.class.floattext : FloatText]]   || change of {{Text}} attribute
Input:onSetText()         || [[manual.html#tek.ui.class.input : Input]]           || change of {{Text}} attribute
Text:onSetText()          || [[manual.html#tek.ui.class.text : Text]]             || change of {{Text}} attribute
Numeric:onSetValue()      || [[manual.html#tek.ui.class.numeric : Numeric]]       || change of the {{Value}} attribute
ScrollBar:onSetValue()    || [[manual.html#tek.ui.class.scrollbar : ScrollBar]]   || change of the {{Value}} attribute

==( adhocclasses : 4.5. Ad-hoc setup of classes )==

To inherit properties and functionality from existing classes and to
consequently reuse existing code, it is often desirable to create new
classes yourself. There are different scopes in which new classes can
be useful:

 * Global classes are written as separate source files, located in
 the system-wide installation path under {{tek/ui/class}} and set up
 using a procedure as described in the [[#classsetup : class setup]]
 section.

 * Application classes are created in the same way, but they are
 located in {{tek/ui/class}} relative to the application's local
 program directory.

 * Another scope is inside a running application or module. We call
 this the ''ad-hoc'' style, because new classes are often created
 out of a spontaneous need.

For the ''ad-hoc'' style, it is not necessary to create a new source
file or module. For example, a Button class can be derived from the
Text class whereever you see fit:

  local Button = ui.Text:newClass { _NAME = "_button" }

''ad-hoc'' classes may be named arbitrarily, but their names should
be prefixed with an underscore to distinguish them from global
classes. You can even do without a name, as tekUI will create one
for you if necessary, but you will find it difficult to reference
such a class in a style sheet.

From this point, the new class can be extended, e.g. for
initializations which turn a Text into a Button:

  function Button.init(self)
    self.Class = "button"
    self.Mode = self.Mode or "button"
    self.KeyCode = true
    return ui.Text.init(self)
  end

As shown in the example, we also passed the call on to our
super class, which we expect to perform the missing initializations.

Finally, a new object from our new class can be created:

  button = Button:new { Text = "_Hello, World!" }

Also refer to the [[manual.html#tek.class : Class reference]] and
the [[#classsetup : Class setup]] section for further information.

---------------------------------------------------------------------

=( developersguide : 5. Developer's Guide )=

==( lifecycle : 5.1. The lifecycle of an element )==

A GUI element is set up in several stages, all of which are initiated
by the tekUI framework. Normally, you do not call any of these
methods yourself (aside from passing a call on to the same method in
your super class):

 - 1. [[Object.init()][manual.html#Object:init]] - gets called when
 an element is created
 - 2. [[Element:connect()][manual.html#Element:connect]] - connects
 the element with a parent element
 - 3. The element's properties are decoded.
 - 4. [[Element:setup()][manual.html#Element:setup]] - registers the
 element with the application
 - 5. [[Area:askMinMax()][manual.html#Area:askMinMax]] - queries the
 element's minimal and maximal dimensions
 - 6. The window is opened.
 - 7. [[Area:show()][manual.html#Area:show]] - gets called when the
 element is about to be shown
 - 8. [[Area:layout()][manual.html#Area:layout]] - layouts the
 element into a rectangle
 - 9. [[Area:draw()][manual.html#Area:draw]] - paints the element
 onto the screen
 - 10. [[Area:hide()][manual.html#Area:hide]] - gets called when the
 element is about to hide
 - 11. The window is closed.
 - 12. [[Element:cleanup()][manual.html#Element:cleanup]] -
 unregisters the element
 - 13. [[Element:disconnect()][manual.html#Element:disconnect]] -
 disconnects the element from its parent

=== Drawing an element ===

In the drawing method, the control flow is roughly as follows:

  function ElementClass:draw()
    if SuperClass.draw(self) then
      -- your rendering here
      return true
    end
  end

There are rare cases in which a class modifies the drawing context,
e.g. by setting a coordinate displacement. Such modifications must
be performed in [[Area:drawBegin()][manual.html#Area:drawBegin]] and
reverted in [[Area:drawEnd()][manual.html#Area:drawEnd]], and the
control flow looks like this:

  function ElementClass:draw()
    if SuperClass.draw(self) then
      if self:drawBegin() then
        -- your rendering here
        self:drawEnd()
      end
      return true
    end
  end

==( debug : 5.2. Debug library )==

The debug library used throughout tekUI is
[[tek.lib.debug][manual.html#tek.lib.debug]]. The default debug
level is 10 ({{ERROR}}). To increase verbosity, set {{level}} to
a lower value, either by modifying {{tek/lib/debug.lua}}, or by
setting it after including the module:

  db = require "tek.lib.debug"
  db.level = db.INFO

See also the module's documentation for redirecting the output.

==( proxied : 5.3. Debugging objects )==

If you wish to use [[#xmlapps : validation of XML files]] against
tekUI's implementation, of if you plan on extending existing
classes or develop your own, you are advised to set the following
configurable parameters in [[tek.class][manual.html#tek.class]],
the base class of all tekUI classes:

  local PROXY = true
  local DEBUG = true

The {{PROXY}} option allows for intercepting read/write accesses to
objects, which will be harnessed by the {{DEBUG}} option for tracking
accesses to uninitialized class members. So whenever a '''nil'''
value is read from or written to an object, this causes {{tek.class}}
to bail out with an error and a meaningful message.

As a result, all member variables must be initialized during
{{new()}} or {{init()}} – or more specifically, before the class
metatable is attached and an object is becoming fully functional.
This will assist in keeping variables neatly together, and you won't
end up in a fluff of variables of limited scope and significance,
getting initialized at random places. This also means that you cannot
assign a distinct meaning to '''nil''' for a class member – you will
have to use '''false''' instead, or find another arrangement. (This
convention of not using '''nil''' for class variables is found
throughout the whole tekUI framework.)

Once your application is tested and ready for deployment, you can
disable {{PROXY}}, as this will improve performance and reduce memory
consumption.

==( classsetup : 5.4. Class setup )==

A class (in a module) is usually set up in a prologue like this:

  local Widget = require "tek.ui.class.widget"
  local Button = Widget.module("tek.ui.class.button", "tek.ui.class.widget")
  Button._VERSION = "Button Widget 1.0"
  ...
  return Button

This first argument to [[Class.module()][manual.html#Class:module]]
is the name of the class to be created. The second argument is the
name of its superclass, the class  to derive the new class from. The
result is the class table that should be the module's return value
also.

Finally, methods in the newly created class may look like this (note
that, thanks to the {{Button}} variable, the second example provides
an implicit {{self}}):

  function Button.new(class, self)
    ...
    return Widget.new(class, self)
  end

  function Button:method()
    Widget.method(self)
    ...
  end

Also, don't forget to add a {{_VERSION}} variable, as it will be used
by the documentation system – see also the next section.

==( classdoc : 5.5. Class documentation system )==

Don't stray off too far from the class setup described in the
previous section, as it contains valuable informations for tekUI's
documentation generator, which tries to assemble a self-contained
class hierarchy from individual class / child class relations.

=== Tokens for markup ===

Aside from the aforementioned {{module}} and {{_VERSION}} keys (see
section [[Class setup][#classsetup]]), the source code parser reacts
on the following tokens.

Long lines of dashes signify the beginnings and endings of comment
blocks that are subject to processing
[[markup notation][manual.html#tek.class.markup]], e.g.

  ----------------------------------------------------------------
  --  OVERVIEW::
  --    Area - implements margins, layouting and drawing
  ----------------------------------------------------------------

The other condition that must be met for the following text to appear
in the documentation is the recognition of either a definition (as
seen in the example) or function marker inside such a comment block.
The template for a definition is this:

  DEFINITION::

And the function template:

  ret1, ret2, ... = function(arg1, arg2, ...): ...

The marker and the following text will then become part of the
documentation. (In other words, by avoiding these markers, it is also
possible to write comment blocks that do not show up in the
documentation.)

Functions inside classes will automatically receive a symbolic name
as their class prefix (from assigning the module table {{_M}} to a
local variable, see [[Class setup][#classsetup]]). Hereinafter, they
can be cross-referenced using the following notations:

  Class:function()
  Class.function()

For further information, consult the sources in the
[[class hierarchy][manual.html]] as examples, and the source code
containing the
[[markup notation reference][manual.html#tek.class.markup]], which
can be found in {{tek.class.markup}}.

==( cssprecedence : 5.6. Stylesheets precedence )==

TekUI uses a cascading stylesheets (CSS) engine, which borrows from
the W3C recommendations. The order of precedence is as follows:

 * Hardcoded class defaults
 * User agent: tekUI's built-in stylesheets, either {{"default"}} or 
 {{"minimal"}}. The implied default is {{"default"}}, unless the
 first word in the {{THEME}} variable is {{"minimal"}}. {{"minimal"}}
 is hardwired into the ui library, while {{"default"}} is an actual
 stylesheet file.
 * User: Names of stylesheet files from the {{THEME}} environment
 variable, e.g. {{"stain gradient"}}. The default is {{"desktop"}}. 
 * Author: Names of stylesheet files specified in
 {{Application.AuthorStyleSheets}}.
 * Author: Styles specified in {{Application.AuthorStyles}}.
 * User important: The {{user.css}} stylesheet.

To shut off interferences from user styles, use a predefined cascade
by overwriting the {{THEME}} variable inside your application, e.g.:

  local ui = require "tek.ui"
  ui.ThemeName = "mycompany" -- also implies "default"
  ui.UserStyles = false -- to also disable the user.css file

See also [[Global Lua runtime arguments][#extraargs]].
