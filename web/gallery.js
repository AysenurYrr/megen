(() => {
  const MOBILE_QUERY = "(max-width: 42rem)";
  const TARGET_ROW_HEIGHT = 190;
  const MAX_ROW_HEIGHT = 240;
  const JUSTIFY_THRESHOLD = 0.9;

  function imageRatio(figure) {
    const image = figure.querySelector("img");
    return image?.naturalWidth && image?.naturalHeight
      ? image.naturalWidth / image.naturalHeight
      : 1;
  }

  function setRowWidths(row, availableWidth, gap, completed) {
    const gapWidth = gap * Math.max(0, row.length - 1);
    const usableWidth = Math.max(0, availableWidth - gapWidth);
    const ratioSum = row.reduce((sum, figure) => sum + imageRatio(figure), 0);
    const justifiedHeight = usableWidth / ratioSum;
    const rowHeight = completed
      ? Math.min(justifiedHeight, MAX_ROW_HEIGHT)
      : Math.min(TARGET_ROW_HEIGHT, MAX_ROW_HEIGHT, justifiedHeight);

    row.forEach((figure) => {
      figure.style.width = `${rowHeight * imageRatio(figure)}px`;
    });
  }

  function resetGallery(gallery) {
    gallery.querySelectorAll(":scope > figure").forEach((figure) => {
      figure.style.removeProperty("width");
    });
  }

  function layoutGallery(gallery) {
    const figures = [...gallery.querySelectorAll(":scope > figure")];
    const availableWidth = gallery.clientWidth;

    if (!figures.length || !availableWidth || matchMedia(MOBILE_QUERY).matches) {
      resetGallery(gallery);
      return;
    }

    const gap = parseFloat(getComputedStyle(gallery).columnGap) || 0;
    let row = [];
    let estimatedWidth = 0;

    figures.forEach((figure) => {
      const nextWidth = TARGET_ROW_HEIGHT * imageRatio(figure);
      estimatedWidth += (row.length ? gap : 0) + nextWidth;
      row.push(figure);

      if (estimatedWidth >= availableWidth * JUSTIFY_THRESHOLD) {
        setRowWidths(row, availableWidth, gap, true);
        row = [];
        estimatedWidth = 0;
      }
    });

    if (row.length)
      setRowWidths(row, availableWidth, gap, false);
  }

  function initGalleries() {
    const galleries = [...document.querySelectorAll(".project-gallery")];
    if (!galleries.length) return;

    let frame = 0;
    const scheduleLayout = () => {
      cancelAnimationFrame(frame);
      frame = requestAnimationFrame(() => {
        galleries.forEach(layoutGallery);
      });
    };

    galleries.forEach((gallery) => {
      gallery.querySelectorAll("img").forEach((image) => {
        if (!image.complete) {
          image.addEventListener("load", scheduleLayout, { once: true });
          image.addEventListener("error", scheduleLayout, { once: true });
        }
      });
    });

    if ("ResizeObserver" in window) {
      const observer = new ResizeObserver(scheduleLayout);
      galleries.forEach((gallery) => observer.observe(gallery));
    } else {
      window.addEventListener("resize", scheduleLayout, { passive: true });
    }

    scheduleLayout();
  }

  function initLightbox() {
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
  }

  initGalleries();
  initLightbox();
})();
