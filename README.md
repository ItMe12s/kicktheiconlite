# Important

Please do not fork this, it's incomplete in terms of feature and the codebase is super outdated from the full version.

## Resources

src/assets/ and not resouces folder
and "resources": { "files": [ ] } because I want to keep the data and textures raw

## To-do

- dedupe
  - ~~overlayLayerRoot~~
  - ~~why was there 2 installPhysicsOverlay~~
  - ~~vfx wrappers not needed in lite~~ actually reworked it a little too

- remove missed deadcode
  - ~~objectCompositeOrder~~
  - ~~OverlayRendering 3 sprite subclasses is just ass~~
  - there's a lot of unused includes

- remove full version stuff that ain't used
  - motion blur api
  - backdrop draw nodes PhysicsOverlay

- kinda important
  - backport/rewrite mod settings stuff
  - unify hide-mod-overlay updates

- i'll do this later
  - refactor PhysicsOverlay
  - refactor OverlayRendering
  - free function for rgb float to draw color
  - less physics for OverlayRendering
