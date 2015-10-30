(TeX-add-style-hook
 "whitepaper"
 (lambda ()
   (TeX-run-style-hooks
    "latex2e"
    "article"
    "art10"
    "fullpage"
    "graphicx"
    "subfig")
   (TeX-add-symbols
    "tbbmap"
    "libcuckoo")
   (LaTeX-add-labels
    "fig:pure-read"
    "fig:pure-insert"
    "fig:mixed-read-insert"
    "fig:mem")))

