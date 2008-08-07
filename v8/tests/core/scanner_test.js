// This tests line terminator followed by a comment.
// See http://b/issue?id=1018745.

var x = []
/* */ var y = 1;

print(y);
