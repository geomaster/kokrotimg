# Building on Cygwin

After countless hours of practically begging the whole gtkmm/gdkmm 
stack to start working on Windows and kludge after kludge to at least 
make the software compilable on MSVC, I have hit a point where 
everything compiles, but when it's run, Gtk::Builder::add_from_resource 
(or add_from_file, for that matter) starts consuming an endless amount 
of memory which is killed off by an out-of-memory condition reported 
by GSplice. I had no intention in diving into the source code of Gtk 
and trying to see where the problem is, so I just gave up and went on 
to build KokrotImg for Cygwin. Granted, this is not a *real* Windows 
build, but it does get the software running, however with a 1990s-era 
look'n'feel and with metrics that report wall-clock instead of CPU time 
(heavily skewed as the screen gets rerendered often, etc., all 
consuming CPU time).

Note that I had no problems with building KokrotImg (the library 
itself) on Windows whatsoever, the problem was with getting gtkmm to
play nice with CMake  and various stuff I have in the tree. This is all 
related to the optional debug visualization tool, KokrotViz, so don't 
be intimidated. And don't try it yourself, it's a hassle, believe me.
I haven't slept in three nights.

## Setting up the environment

It goes like this:

1. Download Cygwin.
2. Install Cygwin. When installing, be sure to mark the following 
   packages:
    * libgtkmm3.0-devel
    * libgtkmm3.0
    * gcc-core
    * gcc-g++
    * make
    * git
    * cmake
    * pkg-config
    * xinit

   These should pull in all the necessary dependencies, like gdk, 
   binutils, blah blah.
3. Open the Cygwin terminal, navigate to a directory of your choice and
   run the following commands
<pre>
$ git clone https://github.com/geomaster/kokrotimg.git
$ cd kokrotimg
</pre>

## Building

Now, run the following:

<pre>
$ mkdir -p build/debug build/release
$ cd build/debug
$ cmake -DCMAKE_BUILD_TYPE=Debug ../..
$ make
$ cd ../release
$ cmake -DCMAKE_BUILD_TYPE=Release ../..
$ make
</pre>

Now you should actually be all set up; the binaries now reside in 
build/debug/bin and build/release/bin.


## Running

1. To run kokrotviz, you need to start X first. Run:
<pre>
$ startxwin
</pre>
   You should see an xterm window show up.
5. Now, we're ready to run kokrotviz:
<pre>
$ cd build/debug <em>[or build/release]</em>
$ export DISPLAY=:0 bin/kokrotviz bin/libkokrotimg.dll
</pre>

It should all be running smoothly. Welcome to 1992!


## Troubleshooting

I've had many issues with running this, but they have been due to my 
leftover files which I have managed to clean up. If any of this fails 
for some reason, try the following stuff: (replace the version numbers 
where appropriate)
<pre>
$ glib-compile-schemas /usr/lib/glib-2.0/schemas
$ gdk-pixbuf-query-loaders > /lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
</pre>
