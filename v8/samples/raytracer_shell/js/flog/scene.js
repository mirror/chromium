/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Scene = Class.create();

Flog.RayTracer.Scene.prototype = {
    camera : null,
    shapes : [],
    lights : [],
    background : null,
    
    initialize : function() {
        this.camera = new Flog.RayTracer.Camera(
            new Flog.RayTracer.Vector(0,0,-5), 
            new Flog.RayTracer.Vector(0,0,1), 
            new Flog.RayTracer.Vector(0,1,0)
        );
        this.shapes = new Array();
        this.lights = new Array();
        this.background = new Flog.RayTracer.Background(new Flog.RayTracer.Color(0,0,0.5), 0.2);
    }
}
