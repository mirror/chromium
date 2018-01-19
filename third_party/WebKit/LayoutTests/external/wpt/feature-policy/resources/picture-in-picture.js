function isPictureInPictureAllowed() {
  return new Promise((resolve, reject) => {
    const video = document.createElement('video');
    video.requestPictureInPicture().then(() => resolve(true), (e) => {
      if (e.name == 'NotAllowedError')
        resolve(true);
      else
        resolve(false);
    });
  });
}
