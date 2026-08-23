(() => {
  const dialog = document.querySelector(".project-lightbox");
  if (!dialog || typeof dialog.showModal !== "function") return;

  const fullImage = dialog.querySelector("img");
  const caption = dialog.querySelector("figcaption");
  const closeButton = dialog.querySelector(".lightbox-close");

  document.addEventListener("click", (event) => {
    const trigger = event.target.closest("[data-lightbox-src]");
    if (!trigger) return;

    fullImage.src = trigger.dataset.lightboxSrc;
    fullImage.alt = trigger.querySelector("img")?.alt || "";
    caption.textContent = trigger.dataset.lightboxCaption || "";
    dialog.showModal();
  });

  closeButton.addEventListener("click", () => dialog.close());

  dialog.addEventListener("click", (event) => {
    if (event.target === dialog) dialog.close();
  });

  dialog.addEventListener("close", () => {
    fullImage.removeAttribute("src");
    fullImage.alt = "";
    caption.textContent = "";
  });
})();
