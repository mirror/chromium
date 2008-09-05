;; After months of asking people what they prefer, we finally
;; have something here that represents the least hated coding
;; style (collectively).  Urgh.


;; To use, put this in your .emacs:
;;
;;    (load-file "/home/${USER}/google/util/coding-style.el")
;;
;; There are some optional features in here as well:
;;
;; If you want the RETURN key to go to the next line and space over
;; to the right place, add this to your .emacs right after the load-file:
;;
;;    (add-hook 'c-mode-common-hook 'google-make-newline-indent)
;;
;; Another feature you may find handy is the Auto mode: once editing a
;; file, use C-c C-a to have Emacs automatically do some newlines.
;; (This annoys most people but it's sorta neat to try.  It's really
;; bad when you're *editing* code but it may be useful when writing
;; new code.)  There's also a Hungry mode: C-c C-d to have the
;; backspace key eat up lots of whitespace at a time.  This annoys
;; some people but it's worth a try.  Both keys can be used again
;; to turn the mode off.  You'll see in the modeline C++/a or C++/h when
;; the mode is turned on.
;;
;; Also useful, perhaps, is fill mode.  In theory, when you use M-q,
;; it will word-wrap your comment paragraphs.

;; Okay we'll use 2 everywhere for Python
(setq py-indent-offset 2)

;; Let's make perl work properly by default, by mflaster@google.com.
;; Never use tabs, and indent by 2 properly.
(defun google-set-perl-style ()
  (interactive)
  (setq perl-indent-level 2)
  (setq perl-continued-statement-offset 2)
  (setq perl-label-offset -2)
  (setq indent-tabs-mode nil))

(add-hook 'perl-mode-hook 'google-set-perl-style)

(setq auto-mode-alist
      (cons '("\\.[ch]$" . c++-mode) auto-mode-alist)) ; .c,.h files in C++ mode

(defconst google-c-style
  '((c-tab-always-indent . t)
    (c-recognize-knr-p . nil)
    (c-enable-xemacs-performance-kludge-p . t) ; speed up indentation in XEmacs
    (c-basic-offset . 2)
    (c-comment-only-line-offset . 0)
    (c-hanging-braces-alist . (
			       (defun-open after)
			       (defun-close before after)
			       (class-open after)
			       (class-close before after)
			       (inline-open after)
			       (inline-close before after)
			       (block-open after)
			       (block-close . c-snug-do-while)
			       (extern-lang-open after)
			       (extern-lang-close after)
			       (statement-case-open after)
			       (substatement-open after)
			       ))
    (c-hanging-colons-alist . (
			       (case-label)
			       (label after)
			       (access-label after)
			       (member-init-intro before)
			       (inher-intro)
			       ))
    (c-hanging-semi&comma-criteria
     . (c-semi&comma-no-newlines-for-oneline-inliners
	c-semi&comma-inside-parenlist
	c-semi&comma-no-newlines-before-nonblanks))
    (c-indent-comments-syntactically-p . nil)
    (comment-column . 40)
    (c-cleanup-list . (brace-else-brace
		       brace-elseif-brace
		       brace-catch-brace
		       empty-defun-braces
		       defun-close-semi
		       list-close-comma
		       scope-operator))
    (c-offsets-alist . (
			(arglist-intro . ++)
                        (func-decl-cont . ++)
                        (member-init-intro . ++)
                        (inher-intro . ++)
			(comment-intro . 0)
			(arglist-close . c-lineup-arglist)
			(topmost-intro . 0)
			(block-open . 0)
			(inline-open . 0)
			(substatement-open . 0)
			(statement-cont . c-lineup-math)
			(label . /)
			(case-label . +)
			(statement-case-open . +)
			(statement-case-intro . +) ; case w/o {
			(access-label . /)
			(innamespace . 0)
			))
    )
  "Google C/C++ Programming Style")

(defun google-set-c-style ()
  (interactive)
  (setq indent-tabs-mode nil)
  (c-add-style "Google" google-c-style t))

(defun google-make-newline-indent ()
  (interactive)
  (define-key c-mode-base-map "\C-m" 'newline-and-indent)
  (define-key c-mode-base-map [ret] 'newline-and-indent))
  
(add-hook 'c-mode-common-hook 'google-set-c-style)

(provide 'google-coding-style)
