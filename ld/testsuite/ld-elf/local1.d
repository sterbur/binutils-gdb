#ld: -shared --version-script local1.map
#readelf: -s --wide
#target: *-*-linux* *-*-gnu* arm*-*-uclinuxfdpiceabi
#xfail: h8300-*-*

#...
    .*: [0-9a-f]* +[0-9a-f]+ +OBJECT +LOCAL +DEFAULT +[0-9] +foo
#...
