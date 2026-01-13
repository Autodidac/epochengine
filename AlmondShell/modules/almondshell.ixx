module;

#include "aengine.hpp"

export module almondshell;



//export import aengine;
// export import other public-facing modules if you want:
// export import aengine.gui;
// export import aengine.core.context;
// primary modules

import aengine.context.renderer;
import aatlas.manager;
import aatlas.texture;
import aspriteregistry;
import atexture;
import aengine.context;
import ascripting.system;
import aengine.taskgraph.dotsystem;
import aengine.updater.system;
import acontext.opengl.context;

// application modules
import aapplicationmodule;
