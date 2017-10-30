function get_worklet(type) {
  if (type == "paint")
    return CSS.paintWorklet;
  if (type == "animation")
    return window.animationWorklet;
  return undefined;
}
