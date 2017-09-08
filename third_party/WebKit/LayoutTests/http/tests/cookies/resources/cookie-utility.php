<?php
parse_str($_SERVER["QUERY_STRING"]);
header("Access-Control-Allow-Origin: http://localhost:8001");
header("Access-Control-Allow-Credentials: true");

function deleteCookie($value, $name)
{
    setcookie($name, "deleted", time() - 86400, '/');
}

if ($queryfunction == "deleteCookies") {
    array_walk($_COOKIE, deleteCookie);
    echo "Deleted all cookies";
    return;
}

if ($queryfunction == "setFooCookie") {
    setcookie("foo", "awesomevalue", time() + 86400, '/');
    echo "Set the foo cookie";
    return;
}

if ($queryfunction == "setFooAndBarCookie") {
    setcookie("foo", "awesomevalue", time() + 86400, '/');
    setcookie("bar", "anotherawesomevalue", time() + 86400, '/');
    echo "Set the foo and bar cookies";
    return;
}

// Default for any other string is echo cookies.
function echoCookie($value, $name)
{
    echo "$name = $value\n";
}

function echoAllCookies()
{
    echo "Cookies are:\n";
    array_walk($_COOKIE, echoCookie);    
}

?>
