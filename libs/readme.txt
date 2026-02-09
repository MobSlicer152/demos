ntdll.dll has the subset of the CRT available to native applications, and the
launcher uses some of those functions. Microsoft only ships the ntdllp.lib
import library that includes those functions in the WDK. Since that's a pain
and also isn't comprehensive, I made this little script. Run it from a VS or Razzle prompt.
