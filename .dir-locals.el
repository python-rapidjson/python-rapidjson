((nil . ((fill-column . 90)
         ;; force reinstall of whitespace font-lock customization
         (eval . (if (functionp 'whitespace-color-on)
		     (whitespace-color-on)))))
 (c++-mode . ((eval . (c-set-style "stroustrup")))))
