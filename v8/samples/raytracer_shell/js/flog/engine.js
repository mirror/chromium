/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Engine = Class.create();

Flog.RayTracer.Engine.prototype = {
    canvas: null, /* 2d context we can render to */
    
    initialize: function(options){
        this.options = Object.extend({  
                canvasHeight: 100,
                canvasWidth: 100,
                pixelWidth: 2,
                pixelHeight: 2,
                renderDiffuse: false,
                renderShadows: false,
                renderHighlights: false,
                renderReflections: false,
                rayDepth: 2
            }, options || {});
    
        this.options.canvasHeight /= this.options.pixelHeight;
        this.options.canvasWidth /= this.options.pixelWidth;
        
        /* TODO: dynamically include other scripts */
    },
    
    setPixel: function(x, y, color){
        var pxW, pxH;
        pxW = this.options.pixelWidth;
        pxH = this.options.pixelHeight;

        if (this.canvas) {
          this.canvas.fillStyle = color.toString();
          this.canvas.fillRect (x * pxW, y * pxH, pxW, pxH);
        } else {
          // print(x * pxW, y * pxH, pxW, pxH);
        }
    },
    
    renderScene: function(scene, canvas){
        /* Get canvas */ 
        if (canvas) {
          this.canvas = canvas.getContext("2d");
        } else {
          this.canvas = null;
        }
        
        var canvasHeight = this.options.canvasHeight;
        var canvasWidth = this.options.canvasWidth;

        for(var y=0; y < canvasHeight; y++){
            for(var x=0; x < canvasWidth; x++){
                var yp = y * 1.0 / canvasHeight * 2 - 1;
          		var xp = x * 1.0 / canvasWidth * 2 - 1;
    
          		var ray = scene.camera.getRay(xp, yp);
          		
          		var color = this.getPixelColor(ray, scene);
            
            	this.setPixel(x, y, color);
            }
        }
    },
    
    getPixelColor: function(ray, scene){
        var info = this.testIntersection(ray, scene, null);
        if(info.isHit){
            var color = this.rayTrace(info, ray, scene, 0);
            return color;
        }
        return scene.background.color;
    },
    
    testIntersection: function(ray, scene, exclude){
        var hits = 0;
        var best = new Flog.RayTracer.IntersectionInfo();
        best.distance = 2000;
        
        for(var i=0; i<scene.shapes.length; i++){
            var shape = scene.shapes[i];
            
            if(shape != exclude){
                var info = shape.intersect(ray);
                if(info.isHit && info.distance >= 0 && info.distance < best.distance){
                    best = info;
                    hits++;
                }
            }
        }
        best.hitCount = hits;
        return best;
    },
    
    getReflectionRay: function(P,N,V){
        var c1 = -N.dot(V);
        var R1 = Flog.RayTracer.Vector.prototype.add(
            Flog.RayTracer.Vector.prototype.multiplyScalar(N, 2*c1),
            V
        );
        return new Flog.RayTracer.Ray(P, R1);
    },
    
    rayTrace: function(info, ray, scene, depth){
        // Calc ambient
        var color = Flog.RayTracer.Color.prototype.multiplyScalar(info.color, scene.background.ambience);
        var oldColor = color;
        var shininess = Math.pow(10, info.shape.material.gloss + 1);
        
        for(var i=0; i<scene.lights.length; i++){
            var light = scene.lights[i];
            
            // Calc diffuse lighting
            var v = Flog.RayTracer.Vector.prototype.subtract(
                                light.position,
                                info.position
                            ).normalize();
            
            if(this.options.renderDiffuse){
                var L = v.dot(info.normal);
                if(L > 0.0){
                    color = Flog.RayTracer.Color.prototype.add(
                                        color,
                                        Flog.RayTracer.Color.prototype.multiply(
                                            info.color,
                                            Flog.RayTracer.Color.prototype.multiplyScalar(
                                                light.color,
                                                L
                                            )
                                        )
                                    );
                }
            }
                
            // The greater the depth the more accurate the colours, but
            // this is exponentially (!) expensive
            if(depth <= this.options.rayDepth){
          // calculate reflection ray
          if(this.options.renderReflections && info.shape.material.reflection > 0)
          {
              var reflectionRay = this.getReflectionRay(info.position, info.normal, ray.direction);
              var refl = this.testIntersection(reflectionRay, scene, info.shape);
              
              if (refl.isHit && refl.distance > 0){
                  refl.color = this.rayTrace(refl, reflectionRay, scene, depth + 1);
              } else {
                  refl.color = scene.background.color;
                        }
                        
                  color = Flog.RayTracer.Color.prototype.blend(
                    color, 
                    refl.color,
                    info.shape.material.reflection
                  );
          }

                // Refraction
                /* TODO */
            }
            
            /* Render shadows and highlights */
            
            var shadowInfo = new Flog.RayTracer.IntersectionInfo();
            
            if(this.options.renderShadows){
                var shadowRay = new Flog.RayTracer.Ray(info.position, v);
                
                shadowInfo = this.testIntersection(shadowRay, scene, info.shape);
                if(shadowInfo.isHit && shadowInfo.shape != info.shape /*&& shadowInfo.shape.type != 'PLANE'*/){
                    var vA = Flog.RayTracer.Color.prototype.multiplyScalar(color, 0.5);
                    var dB = (0.5 * Math.pow(shadowInfo.shape.material.transparency, 0.5));
                    color = Flog.RayTracer.Color.prototype.addScalar(vA,dB);
                }
            }
      
      // Phong specular highlights
      if(this.options.renderHighlights && !shadowInfo.isHit && info.shape.material.gloss > 0){
        var Lv = Flog.RayTracer.Vector.prototype.subtract(
                            info.shape.position,
                            light.position
                        ).normalize();
        
        var E = Flog.RayTracer.Vector.prototype.subtract(
                            scene.camera.position,
                            info.shape.position
                        ).normalize();
        
        var H = Flog.RayTracer.Vector.prototype.subtract(
                            E,
                            Lv
                        ).normalize();
        
        var glossWeight = Math.pow(Math.max(info.normal.dot(H), 0), shininess);
        color = Flog.RayTracer.Color.prototype.add(
                            Flog.RayTracer.Color.prototype.multiplyScalar(light.color, glossWeight),
                            color
                        );
      }
        }
        color.limit();
        return color;
    }
};
