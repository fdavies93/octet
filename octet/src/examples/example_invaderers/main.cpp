////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Invaderers - will space be free of 'em?
//

#include "../../octet.h"

#include "invaderers_app.h"
//note: example_invaderers.h does NOTHING
//this file is a /very/ basic instantiation file
//the important parts are in invaderers_app.h and texture_shader.h

/// Create a box with octet
int main(int argc, char **argv) {
  // set up the platform.
  octet::app::init_all(argc, argv);

  // our application.
  octet::invaderers_app app(argc, argv);
  app.init();

  // open windows
  octet::app::run_all_apps();
}

