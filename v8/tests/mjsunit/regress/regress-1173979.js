// Copyright 2008 Google Inc. All Rights Reserved.
// Ensure that null only equals null and undefined, also for variables.

var null_var = null;
var undef_var = [][0];
var boolean_var = false;
var number_var = 0;
var string_var = "";
var object_var = { foo : 0 };

assertTrue(null_var == null_var);
assertTrue(null_var == undef_var);
assertTrue(null_var != boolean_var);
assertTrue(null_var != number_var);
assertTrue(null_var != string_var);
assertTrue(null_var != object_var);

assertTrue(undef_var == null_var);
assertTrue(boolean_var != null_var);
assertTrue(number_var != null_var);
assertTrue(string_var != null_var);
assertTrue(object_var != null_var);
