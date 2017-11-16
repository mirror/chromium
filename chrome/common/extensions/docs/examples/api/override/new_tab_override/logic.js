const kCoffee = [
    {name: "Espresso",
    image: "/images/espresso.png"},
    {name: "Macchiato",
    image: "/images/macchiato.png"},
    {name: "Cup O'Chrome",
    image: "/images/cupOchrome.png"}]


var random = kCoffee[Math.floor(Math.random() * kCoffee.length)];
console.log(random);
document.getElementById("coffeeTitle")
    .textContent = random.name;
document.getElementById("coffeeImage")
    .setAttribute("src", random.image)
