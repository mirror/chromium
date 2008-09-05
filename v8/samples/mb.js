// Mandelbrot benchmark from: http://www.rahul.net/rhn/bench.js.html

function qset(xx, yy, u, v) {
	var n 
	var t, xsqr, ysqr
	var lim = 100
	var x = xx
	var y = yy
	
	xsqr = x*x
	ysqr = y*y
    
	for (n=0; (n < lim) && (xsqr+ysqr < 4.0); n++) { 
		t = xsqr - ysqr + u
		y = 2.0*x*y + v
		x = t
		xsqr = t*t
		ysqr = y*y
	}
    
	return(n)
}


function mb(niter) {
	var dots = 0;
	var res = niter;
	var a1 = -2.50;
	var b1 = -1.75;
	var s  =  3.50;   /* side */
	var x = 0;
	var y = 0;
	var g  = s/res;   /* gap  */
	var i,j,a,b,k;
	
    var sum = 0;  // SRM
	for (j=0,b=b1; j<res; b+=g,j++) {
		for (i=0,a=a1; i<res; a+=g,i++) {
			k = qset(x,y,a,b);
			if (k > 90) dots++;
            sum += k;   // SRM
		}
	}
    print("sum: " + sum);
	return(dots);
}


var start = new Date();
mb(800);
print("Time (Mandelbrot 800): " + (new Date() - start) + " ms.");
