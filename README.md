# TU Wien - CGUE SS_2018

how to install deps:

- install "bullet", "glfw-x11", "vulkan-headers", "vulkan-icd-loader", "vulkan-extra-layers", "spirv-tools", "glslang", "glm"
- `mkdir build`
- `cd build`
- `cmake ..`
- `make`
- `bin/heikousen`

installing deps on windows:

- Install cmake and have it in PATH
- git submodule init
- git submodule update
- Start Visual Studio Dev Console x64 run ```call build_deps```

TODO:
- plan architecture
- do concept

0. configuration
1. hello world vulkan
1. val layers
1. refactor for hotkey use
1. pipeline rebuild (wireframe etc)
1. bullet colldet/collresp test (= game over condition)
1. script engine
1. movement in planning phase, bullet based (= win condition)
1. movement tracking / replay system
1. model loading
1. textures (multiple maps)
1. level design (= moving objects)
1. GAME DAY
1. Frustum Culling
1. Hierarchical Animation Using Physics Engine
1. HDR
1. HUD
1. EFFECTS
1. level design
1. sound design
1. polishing


## game ideas

heikousen (= parrallel lines)
- 3d movement (like elite dangerous)
- plan your movement through the (static) level
- then see your move played out with all the dynamic elements active

gameplay ideas
- have static elements already present in planning
- have elements react to your previous moves
- have your previous ghost's explosions influence the current run
- score?
- have some kinda interactivity during the run (one shot, shield, particle elimination, automatic course correction)


## effects list

mandatory:
- Light Mapping (optional in separate textures or calculation at startup)
- Physics Interaction (collision detection + collision response)
- Heads-Up Display using Blending

for sure:
- (0.5) + separate texture light maps 
- (1) scripting language?
- (1) HDR rendering and tone mapping

considered:
- (1) GPU particle system
- (1.5) blobby objects / metaballs
    - https://matiaslavik.wordpress.com/computer-graphics/metaball-rendering/
- (1) BSP trees (for transparent objets)?
- (1) kd tree (for culling)
- (1) procedural textures
- (1) video textures
- (0.5) simple normal mapping

PBR shader?
- https://learnopengl.com/PBR/Theory
- https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/README.md 
(holy shit this looks amazing)


## software used:

- cmake (build tool)
- vulkan (graphics)
- bullet (physics)
- sound?


## resources

scripting
- https://github.com/underscorediscovery/v8-tutorials
- http://syskall.com/how-to-roll-out-your-own-javascript-api-with


sound
- https://www.reddit.com/r/gamedev/comments/28h6wf/c_audio_libraries/
- https://www.reddit.com/r/GameAudio/comments/3ipbys/fmod_vs_wwise/
- http://danikog.github.io/GameAudioTable/
- AAA VR sound
    - FMOD: https://fmod.com/api
        - Wwise: https://www.audiokinetic.com/products/wwise/
        - plus google resonance audio
            - Resources: https://developers.google.com/resonance-audio/
            - DEMO: https://www.youtube.com/watch?v=IYdx9cnHN8I
    - indie sound
        - fabric + elias
    - old school
        - BASS: https://www.un4seen.com/ 
        - OpenAL: https://github.com/kcat/openal-soft


bullet
- CMAKE: https://cmake.org/cmake/help/v3.0/modgoogle vr audioule/FindBullet.html
- https://www.sjbaker.org/wiki/index.php?title=Physics_-_Bullet_HelloWorld_example
- http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Hello_World
- http://www.bulletphysics.org/mediawiki-1.5.8/index.php/Creating_a_project_from_scratch


vulkan
- https://github.com/SaschaWillems/Vulkan
- 


cmake


models
- https://sketchfab.com/
- https://www.remix3d.com/discover
- https://www.blender.org/2-8/
- I have some more resources at home, please remind me to add them here
- level editor: gtkRadiant


### raffy recommends:

json parsing (for configs)
- "json for modern c++" @github



