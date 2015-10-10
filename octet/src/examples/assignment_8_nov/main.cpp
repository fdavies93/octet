////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Text overlay
//

#define OCTET_BULLET 1

#include <iostream>
#include <string>
#include <list>

#include "../../octet.h"

#include "assignment_8_nov.h"

/// Create a box with octet
int main(int argc, char **argv) {
  // set up the platform.
  octet::app::init_all(argc, argv);

  // our application.
  octet::assignment_8_nov app(argc, argv);
  app.init();

  // open windows
  octet::app::run_all_apps();
}


