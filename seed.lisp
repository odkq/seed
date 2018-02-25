; Some handy functions
(defun not (e) (if (eq e t) () t))
(defun list (x . y) (cons x y))

; let for temporary variables with scope
(defmacro let (var val . body)
  (cons (cons 'lambda (cons (list var) body))
    (list val)))

; do times for easy iteration
(defmacro dotimes (expression body)
  (list 'let 'i '0
    (list 'while (list '< 'i expression)
    body
    (list 'setq 'i (list '+ 'i '1)))))

(defmacro progn (expr . rest)
    (list (cons 'lambda (cons () (cons expr rest)))))

; VT100 escape sequences. See http://www.termsys.demon.co.uk/vtansi.htm
(define esc (number->byte 27))
(defun curnopar (s) (string-append esc s))
(defun curonepar (n s) (string-append esc "[" (number->string n) s))
(defun curtwopar (a b s) (string-append esc "[" (number->string a) ";"
                          (number->string b) s))
(defun cur-home () (curnopar "[H"))
(defun cur-xy (x y) (curtwopar y x "f"))
(defun cur-up (y) (curonepar y "A"))
(defun cur-down (y) (curonepar y "B"))
(defun cur-left (y) (curonepar y "D"))
(defun cur-right (y) (curnopar y "C"))
(defun cur-save () (curnopar "[s"))
(defun cur-restore () (curnopar "[u"))
(defun clr-screen () (curnopar "[2J"))
(defun clr-to-eol () (curnopar "[K"))
(defun clear-to-sol () (curnopar "[1K"))
(defun clear-row () (curnopar  "[2K"))
(defun clear-above () (curonepar "[1J"))
(defun clear-below () (curonepar "[J"))
(defun clear-attribs () (curonepar "[0m"))
(defun scroll (s e) (curtwopar s e "r"))
(defun scroll-down () (curonepar "D"))
(defun scroll-up () (curonepar "M"))

(define width (car (screen-size)))
(define height (car(cdr(screen-size))))

(prn (clr-screen))

(define do-loop 1)
(define y 0)
(while (= do-loop 1)
  (let key (read-character)
    (if (= key 113)
      (setq do-loop 0))
    (prn (cur-xy 5 y))
    (prn "    ")
    (prn (cur-xy 5 y))
    (prn (number->string key)))
    (setq y (+ y 1)))

;(while (< y height)
;    (while (< x (+ width 1))
;           (prn (cur-xy x y))
;           (if (= y (- height 2)) (prn "─") (prn "·"))
;           (setq x (+ x 1)))
;    ; (sleep 1)
;    (setq x 0)
;    (setq y (+ y 1)))
;(sleep 5)
;(define i 0)
;(while (not (= i 20))
;     (prn (cur-xy 0 i))
;     (prn "Testing VT100 escape sequences")
;     (sleep 1)
;     (define i (+ i 1)))
