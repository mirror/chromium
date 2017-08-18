registerPaint('geometry', class {
    paint(ctx, geom) {
        ctx.strokeStyle = 'green';
        ctx.lineWidth = 4;
        ctx.strokeRect(0, 0, geom.width, geom.height);
    }
});
