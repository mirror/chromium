// Shell for raytracer
// Run with:
/*
v8 js/prototype.js js/flog/color.js js/flog/light.js js/flog/vector.js js/flog/ray.js \
   js/flog/scene.js js/flog/material/basematerial.js js/flog/material/solid.js \
   js/flog/material/chessboard.js js/flog/shape/baseshape.js js/flog/shape/sphere.js \
   js/flog/shape/plane.js js/flog/intersectioninfo.js js/flog/camera.js \
   js/flog/background.js js/flog/engine.js shell.js

*/

function renderScene(){ 
    var scene = new Flog.RayTracer.Scene();
    
    scene.camera = new Flog.RayTracer.Camera(
                        new Flog.RayTracer.Vector(0, 0, -15),
                        new Flog.RayTracer.Vector(-0.2, 0, 5),
                        new Flog.RayTracer.Vector(0, 1, 0)
                    );
    
    scene.background = new Flog.RayTracer.Background(
                                new Flog.RayTracer.Color(0.5, 0.5, 0.5),
                                0.4
                            );
    
    var sphere = new Flog.RayTracer.Shape.Sphere(
        new Flog.RayTracer.Vector(-1.5, 1.5, 2),
        1.5,
        new Flog.RayTracer.Material.Solid(
            new Flog.RayTracer.Color(0,0.5,0.5),
            0.3,
            0.0,
            0.0,
            2.0
        )
    );
    
    var sphere1 = new Flog.RayTracer.Shape.Sphere(
        new Flog.RayTracer.Vector(1, 0.25, 1),
        0.5,
        new Flog.RayTracer.Material.Solid(
            new Flog.RayTracer.Color(0.9,0.9,0.9),
            0.1,
            0.0,
            0.0,
            1.5
        )
    );
    
    var plane = new Flog.RayTracer.Shape.Plane(
                                new Flog.RayTracer.Vector(0.1, 0.9, -0.5).normalize(),
                                1.2,
                                new Flog.RayTracer.Material.Chessboard(
                                    new Flog.RayTracer.Color(1,1,1),
                                    new Flog.RayTracer.Color(0,0,0),
                                    0.2,
                                    0.0,
                                    1.0,
                                    0.7
                                )
                            );
    
    scene.shapes.push(plane);
    scene.shapes.push(sphere);
    scene.shapes.push(sphere1);

    var light = new Flog.RayTracer.Light(
        new Flog.RayTracer.Vector(5, 10, -1),
        new Flog.RayTracer.Color(0.8, 0.8, 0.8)
    );
    
    var light1 = new Flog.RayTracer.Light(
        new Flog.RayTracer.Vector(-3, 5, -15),
        new Flog.RayTracer.Color(0.8, 0.8, 0.8),
        100
    );
    
    scene.lights.push(light);
    scene.lights.push(light1);
    
    var imageWidth = 500; // $F('imageWidth');
    var imageHeight = 500; // $F('imageHeight');
    var pixelSize = "5,5".split(','); //  $F('pixelSize').split(',');
    var renderDiffuse = true; // $F('renderDiffuse');
    var renderShadows = true; // $F('renderShadows');
    var renderHighlights = true; // $F('renderHighlights');
    var renderReflections = true; // $F('renderReflections');
    var rayDepth = 2;//$F('rayDepth');
    
    var raytracer = new Flog.RayTracer.Engine(
        {
            canvasWidth: imageWidth,
            canvasHeight: imageHeight,
            pixelWidth: pixelSize[0],
            pixelHeight: pixelSize[1],
            "renderDiffuse": renderDiffuse,
            "renderHighlights": renderHighlights,
            "renderShadows": renderShadows,
            "renderReflections": renderReflections,
            "rayDepth": rayDepth
        }
    );
    
    print("Rendering start");
    var start = new Date();
    raytracer.renderScene(scene, null, 0);
    var duration = new Date() - start;
    print("Rendering length: " + duration + " ms.");
}


var document = null;
print("Raytracer shell");

renderScene();
