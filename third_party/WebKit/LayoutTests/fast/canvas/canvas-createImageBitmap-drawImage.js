function jsWrapperClass(node)
{
    // returns the ClassName of node
    if (!node)
        return "[null]";
    var string = Object.prototype.toString.apply(node);

    // string will be of the form [object ClassName]
    return string.substr(8, string.length - 9);
}

function shouldBeType(expression, className)
{
    assert_equals(jsWrapperClass(expression), className);
}

function promiseRejectedUnexpectedly() {
    assert_true(false, "createImageBitmap promise rejected.");
}

function promiseResolvedUnexpectedly(message) {
    assert_true(false, message);
}

function shouldBeRed(ctx, x, y) {
    d = ctx.getImageData(x, y, 1, 1).data;
    assert_array_equals(d, [255, 0, 0, 255]);
}

function shouldBeGreen(ctx, x, y) {
    d = ctx.getImageData(x, y, 1, 1).data;
    assert_array_equals(d, [0, 255, 0, 255]);
}

function shouldBeBlue(ctx, x, y) {
    d = ctx.getImageData(x, y, 1, 1).data;
    assert_array_equals(d, [0, 0, 255, 255]);
}

function shouldBeBlack(ctx, x, y) {
    d = ctx.getImageData(x, y, 1, 1).data;
    assert_array_equals(d, [0, 0, 0, 255]);
}

function shouldBeClear(ctx, x, y) {
    // should be transparent black pixels
    d = ctx.getImageData(x, y, 1, 1).data;
    assert_array_equals(d, [0, 0, 0, 0]);
}

function clearContext(ctx, width, height) {
    ctx.clearRect(0, 0, width, height);
}

function drawPattern(ctx) {
    // Draw a four-color pattern
    ctx.beginPath();
    ctx.fillStyle = "rgb(255, 0, 0)";
    ctx.fillRect(0, 0, 10, 10);
    ctx.fillStyle = "rgb(0, 255, 0)";
    ctx.fillRect(10, 0, 10, 10);
    ctx.fillStyle = "rgb(0, 0, 255)";
    ctx.fillRect(0, 10, 10, 10);
    ctx.fillStyle = "rgb(0, 0, 0)";
    ctx.fillRect(10, 10, 10, 10);
}

//--------------- Test routines for everything except video --------------------

function checkNoCrop(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");
    assert_equals(imageBitmap.width, 20);
    assert_equals(imageBitmap.height, 20);

    // should be drawn to (0, 0), (20, 20)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeRed(ctx, 9, 9);
    shouldBeGreen(ctx, 11, 9);
    shouldBeBlue(ctx, 9, 11);
    shouldBeBlack(ctx, 11, 11);
    shouldBeBlack(ctx, 19, 19);
    shouldBeClear(ctx, 1, 21);
    shouldBeClear(ctx, 21, 1);
    shouldBeClear(ctx, 21, 21);

    // shrunk to (0, 0), (10, 10)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0, 10, 10);
    shouldBeRed(ctx, 4, 4);
    shouldBeGreen(ctx, 6, 4);
    shouldBeBlue(ctx, 4, 6);
    shouldBeBlack(ctx, 6, 6);
    shouldBeBlack(ctx, 9, 9);
    shouldBeClear(ctx, 1, 11);
    shouldBeClear(ctx, 11, 1);
    shouldBeClear(ctx, 11, 11);

    // shrunk to (10, 10), (20, 20)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 10, 10, 10, 10);
    shouldBeRed(ctx, 14, 14);
    shouldBeGreen(ctx, 16, 14);
    shouldBeBlue(ctx, 14, 16);
    shouldBeBlack(ctx, 16, 16);
    shouldBeBlack(ctx, 19, 19);
    shouldBeClear(ctx, 11, 21);
    shouldBeClear(ctx, 21, 11);
    shouldBeClear(ctx, 21, 21);

    // black should be drawn to (10, 10), (20, 20)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 10, 10, 10, 10, 10, 10, 10, 10);
    shouldBeClear(ctx, 9, 9);
    shouldBeBlack(ctx, 11, 11);
    shouldBeBlack(ctx, 19, 19);
    shouldBeClear(ctx, 1, 21);
    shouldBeClear(ctx, 21, 1);
    shouldBeClear(ctx, 21, 21);
}

function checkCrop(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");
    assert_equals(imageBitmap.width, 10);
    assert_equals(imageBitmap.height, 10);

    // red should be drawn to (0, 0), (10, 10)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeRed(ctx, 0, 1);
    shouldBeRed(ctx, 1, 0);
    shouldBeRed(ctx, 1, 1);
    shouldBeRed(ctx, 9, 9);
    shouldBeClear(ctx, 12, 12);
    shouldBeClear(ctx, 1, 12);
    shouldBeClear(ctx, 12, 1);

    // red should be drawn to (0, 0), (20, 20)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0, 20, 20);
    shouldBeRed(ctx, 1, 1);
    shouldBeRed(ctx, 18, 18);
    shouldBeClear(ctx, 22, 22);
    shouldBeClear(ctx, 1, 22);
    shouldBeClear(ctx, 22, 1);
}

function checkCropCenter(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");
    assert_equals(imageBitmap.width, 10);
    assert_equals(imageBitmap.height, 10);

    // should be drawn to (0, 0), (10, 10) with all four colors
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeRed(ctx, 4, 4);
    shouldBeGreen(ctx, 6, 4);
    shouldBeBlue(ctx, 4, 6);
    shouldBeBlack(ctx, 6, 6);
    shouldBeBlack(ctx, 9, 9);
    shouldBeClear(ctx, 11, 11);
    shouldBeClear(ctx, 1, 11);
    shouldBeClear(ctx, 11, 1);

    // should be drawn to (0, 0), (20, 20) with all four colors
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0, 20, 20);
    shouldBeRed(ctx, 8, 8);
    shouldBeGreen(ctx, 11, 8);
    shouldBeBlue(ctx, 8, 11);
    shouldBeBlack(ctx, 11, 11);
    shouldBeBlack(ctx, 18, 18);
    shouldBeClear(ctx, 22, 22);
    shouldBeClear(ctx, 1, 21);
    shouldBeClear(ctx, 21, 1);
}

function checkCropRight(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");
    assert_equals(imageBitmap.width, 10);
    assert_equals(imageBitmap.height, 10);

    // black should be drawn to (0, 0), (10, 10)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeBlack(ctx, 1, 1);
    shouldBeBlack(ctx, 9, 9);
    shouldBeClear(ctx, 11, 11);
    shouldBeClear(ctx, 1, 11);
    shouldBeClear(ctx, 11, 1);
}

function checkOverCrop(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");
    assert_equals(imageBitmap.width, 20);
    assert_equals(imageBitmap.height, 20);

    // should be drawn to (0, 0), (20, 20)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeRed(ctx, 9, 9);
    shouldBeGreen(ctx, 11, 9);
    shouldBeBlue(ctx, 9, 11);
    shouldBeBlack(ctx, 11, 11);
    shouldBeBlack(ctx, 19, 19);
    shouldBeClear(ctx, 1, 21);
    shouldBeClear(ctx, 21, 1);
    shouldBeClear(ctx, 21, 21);

    // shrunk to (0, 0), (10, 10)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0, 10, 10);
    shouldBeRed(ctx, 4, 4);
    shouldBeGreen(ctx, 6, 4);
    shouldBeBlue(ctx, 4, 6);
    shouldBeBlack(ctx, 6, 6);
    shouldBeBlack(ctx, 9, 9);
    shouldBeClear(ctx, 1, 11);
    shouldBeClear(ctx, 11, 1);
    shouldBeClear(ctx, 11, 11);
}

function checkOverCropRight(ctx, imageBitmap) {
    assert_equals(imageBitmap.width, 10);
    assert_equals(imageBitmap.height, 10);

    // black should be drawn to (0, 0), (10, 10)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeBlack(ctx, 1, 1);
    shouldBeBlack(ctx, 9, 9);
    shouldBeClear(ctx, 11, 11);
    shouldBeClear(ctx, 1, 11);
    shouldBeClear(ctx, 11, 1);

    // black should be drawn to (0, 0), (4, 4)
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0, 4, 4);
    shouldBeBlack(ctx, 1, 1);
    shouldBeBlack(ctx, 3, 3);
    shouldBeClear(ctx, 5, 5);
    shouldBeClear(ctx, 1, 5);
    shouldBeClear(ctx, 5, 1);

    // nothing should be drawn
    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 10, 10, 20, 20, 0, 0, 20, 20);
    shouldBeClear(ctx, 1, 1);
    shouldBeClear(ctx, 3, 3);
    shouldBeClear(ctx, 5, 5);
    shouldBeClear(ctx, 1, 5);
    shouldBeClear(ctx, 5, 1);
}

function checkBlankImageBitmap(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");

    clearContext(ctx, 50, 50);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeClear(ctx, 1, 1);
    shouldBeClear(ctx, 9, 9);
    shouldBeClear(ctx, 11, 11);
    shouldBeClear(ctx, 21, 21);
}

//------------------------ Test routines for video -----------------------------

function compareTwoCanvases(sx, sy, sw, sh, ctx1, ctx2)
{
    var data1 = ctx1.getImageData(sx, sy, sw, sh).data;
    var data2 = ctx2.getImageData(sx, sy, sw, sh).data;
    // assert_array_approx_equals(data1, data2, 80);
    var dataMatched = true;
    for (var i = 0; i < data1.length; i++) {
        // data and data1 are strictly the same under software rendering,
        // but significantly different on GPU.
        if (Math.abs(data1[i] - data2[i]) > 80) {
            dataMatched = false;
            break;
        }
    }
    assert_true(dataMatched);
}

function checkNoCropVideo(ctx1, ctx2, video, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");

    // should be drawn to (0, 0), (352, 288)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 0, 0);
    ctx2.drawImage(video, 0, 0);
    compareTwoCanvases(0, 0, 352, 288, ctx1, ctx2);

    // shrunk to (0, 0), (500, 500)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 0, 0, 500, 500);
    ctx2.drawImage(video, 0, 0, 500, 500);
    compareTwoCanvases(0, 0, 500, 500, ctx1, ctx2);

    // shrunk to (100, 100), (380, 380)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 100, 100, 380, 380);
    ctx2.drawImage(video, 100, 100, 380, 380);
    compareTwoCanvases(100, 100, 380, 380, ctx1, ctx2);

    // black should be drawn to (100, 100), (200, 200)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 100, 100, 100, 100, 100, 100, 100, 100);
    ctx2.drawImage(video, 100, 100, 100, 100, 100, 100, 100, 100);
    compareTwoCanvases(100, 100, 100, 100, ctx1, ctx2);

    // nothing should be drawn
    clearContext(ctx1, 500, 500);
    ctx1.drawImage(imageBitmap, 400, 300, 200, 200, 100, 100, 100, 100);
    shouldBeClear(ctx1, 10, 10);
    shouldBeClear(ctx1, 80, 80);
    shouldBeClear(ctx1, 150, 150);
    shouldBeClear(ctx1, 210, 210);
}

function checkCropVideo(ctx1, ctx2, video, imageBitmap, sx, sy, sw, sh) {
    shouldBeType(imageBitmap, "ImageBitmap");

    // should be drawn to (0, 0), (100, 100)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 0, 0);
    ctx2.drawImage(video, sx, sy, sw, sh, 0, 0, sw, sh);
    compareTwoCanvases(0, 0, 100, 100, ctx1, ctx2);
}

function checkOverCropVideo(ctx1, ctx2, video, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");

    // should be drawn to (0, 0), (352, 288)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 0, 0);
    ctx2.drawImage(video, 0, 0, 352, 288);
    compareTwoCanvases(0, 0, 500, 500, ctx1, ctx2);
}

function checkOverCropRightVideo(ctx1, ctx2, video, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");

    // should be drawn to (0, 0), (252, 188)
    clearContext(ctx1, 500, 500);
    clearContext(ctx2, 500, 500);
    ctx1.drawImage(imageBitmap, 0, 0);
    ctx2.drawImage(video, 100, 100, 252, 188, 0, 0, 252, 188);
    compareTwoCanvases(0, 0, 252, 188, ctx1, ctx2);;
}

function checkBlankImageBitmapVideo(ctx, imageBitmap) {
    shouldBeType(imageBitmap, "ImageBitmap");

    clearContext(ctx, 500, 500);
    ctx.drawImage(imageBitmap, 0, 0);
    shouldBeClear(ctx, 10, 10);
    shouldBeClear(ctx, 90, 90);
    shouldBeClear(ctx, 110, 110);
    shouldBeClear(ctx, 210, 210);
}
