// Does not print the right error message. When
// printing the error message the string is terminated
// when the 0-character is reached. 

var string = 'a' + String.fromCharCode(0) + 'xx';
string.doesNotExist()
