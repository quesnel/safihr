Requirements
============

vle      1.1  http://www.vle-project.org
boost    1.49 http://www.boost.org
cmake    2.8  http://www.cmake.org


Installation
============

The safihr package requires vle.output and vle.extension.decision packages.

    vle --package=output configure build
    vle --package=vle.extension.decision configure build
    vle --package=safihr configure build
