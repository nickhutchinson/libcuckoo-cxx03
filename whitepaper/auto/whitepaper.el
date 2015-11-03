(TeX-add-style-hook
 "whitepaper"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-package-options
                     '(("hyperref" "colorlinks" "linkcolor=blue")))
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperref")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperimage")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperbaseurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "nolinkurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "path")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "path")
   (TeX-run-style-hooks
    "latex2e"
    "article"
    "art10"
    "fullpage"
    "graphicx"
    "subfig"
    "hyperref")
   (TeX-add-symbols
    "tbbmap"
    "libcuckoo")
   (LaTeX-add-labels
    "fig:pure-read"
    "fig:pure-insert"
    "fig:pure-insert-one-numa"
    "fig:mixed-read-insert"
    "fig:mem")))

