; Some handy functions
(defun not (e) (if (eq e t) () t))
(defun list (x . y) (cons x y))

;(defun nth (lis n)    ;;  Returns nth element of lis.
;  (if (= n 0)
;    (car lis)
;    (nth (cdr lis) (- n 1))))


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

(defmacro when (expr . body)
  (cons 'if (cons expr (list (cons 'progn body)))))

; non recursive definition of nth
(defun nth (lis n)
  (dotimes n (setq lis (cdr lis)))
  (car lis))

;;; Applies each element of lis to fn, and returns their return values as a
;;; list.
(defun map (lis fn)
  (when lis
    (cons (fn (car lis))
    (map (cdr lis) fn))))

;; ANSI/VT100 escape sequences. See http://www.termsys.demon.co.uk/vtansi.htm
;; and https://en.wikipedia.org/wiki/ANSI_escape_code
(define esc (number->byte 27))
(defun curnopar (s) (string-append esc s))
(defun curonepar (n s) (string-append esc "[" (number->string n) s))
(defun curtwopar (a b s) (string-append esc "[" (number->string a) ";"
                          (number->string b) s))
(defun curthreepar (a b c s) (string-append esc "[" (number->string a) ";"
                               (number->string b) ";" (number->string c) s))
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
(defun clear-attribs () (curnopar "[0m"))
(defun scroll (s e) (curtwopar s e "r"))
(defun scroll-down () (curonepar "D"))
(defun scroll-up () (curonepar "M"))
(defun set-one-attrib (a) (curonepar a "m"))
(defun set-two-attrib (a b) (curtwopar a b "m"))
(defun set-fg (a) (curthreepar 38 5 a "m"))
(defun set-bg (a) (curthreepar 48 5 a "m"))
(defun prn-xy (x y s) (prn (cur-xy x y)) (prn s))

(define width (car (screen-size)))
(define height (car(cdr(screen-size))))

(define horizontal-line "─")
(define vertical-line "│")
(define corner-top-left "┌")
(define corner-top-right "┐")
(define corner-bottom-left "└")
(define corner-bottom-right "┘")

(defun reset-screen ()
  (while (< y height)
    (while (< x (+ width 1))
      (prn (cur-xy x y))
      (if (= y (- height 2)) (prn "─") (prn "·"))
        (setq x (+ x 1)))
     (sleep 1)
     (setq x 0)
     (setq y (+ y 1))))

(defun draw-buffer (w-x w-y w-w w-h delta-x delta-y buffer)
  (let tx 0
    (let ty 0
      (while (< tx (+ w-w 1))
        (while (< ty (+ w-h 1))
          (prn-xy (+ w-x tx) (+ w-y ty) "·")
          (setq ty (+ ty 1)))
        (setq ty 0)
        (setq tx (+ tx 1))))))

(defun draw-hline (x y width)
  (let tx 0
    (while (< tx (+ width 1))
      (prn-xy (+ tx x) y horizontal-line)
      (setq tx (+ tx 1)))))

(defun draw-vline (x y height)
  (let ty 0
    (while (< ty (+ height 1))
      (prn-xy x (+ ty y) vertical-line)
      (setq ty (+ ty 1)))))

(defun draw-box (x y width height)
  (draw-hline (+ x 1) y (- width 2))
  (draw-hline (+ x 1) (+ y height) (- width 2))
  (draw-vline x (+ y 1) (- height 2))
  (draw-vline (+ x width) (+ y 1) (- height 2))
  (prn-xy x y corner-top-left)
  (prn-xy (+ x width) y corner-top-right)
  (prn-xy x (+ y height) corner-bottom-left)
  (prn-xy (+ x width) (+ y height) corner-bottom-right))

(defun test-attributes (n)
  (let attribs (list (cons 1 "bright") (cons 2 "dim") (cons 4 "underscore")
                     (cons 5 "blinking") (cons 7 "reverse") (cons 8 "hidden"))
    (map attribs (lambda (x)
                   (prn-xy 40 n "->")
                   (prn (set-one-attrib (car x)))
                   (prn (cdr x))
                   (prn (clear-attribs))
                   (setq n (+ n 1))))))

(defun test-colors ()
  (let fg_color 30
    (while (< fg_color 38)
      (let bg_color 40
        (while (< bg_color 48)
          (let x (+ (- bg_color 40))
            (let y (- fg_color 30)
              (prn (set-two-attrib fg_color bg_color))
              (prn-xy x y "X")
              (prn (clear-attribs))))
          (setq bg_color (+ bg_color 1))))
      (setq fg_color (+ fg_color 1)))))

(defun test-colors-256 ()
  (let color 0
    (while (< color 256)
      (let x (+ 1 (* (% color 16) 4))
        (let y (+ 1 (/ color 16))
          (prn (set-fg color))
          (prn-xy x y (number->string color))
          (prn (clear-attribs))
          (prn (set-fg 15))
          (prn (set-bg color))
          (prn-xy x (+ y 16) (number->string color))
          (prn (clear-attribs))))
      (setq color (+ color 1)))))

; Entry point
(if (= (into-repl) 0)
  ((lambda ()
    (raw-mode)
    (prn (clr-screen))
    (define do-loop 1)
    (test-attributes 35)
    (test-colors-256)
    (while (= do-loop 1)
      (let key (read-character)
        (if (= key 113)
          (setq do-loop 0))
        (draw-buffer 5 5 60 25 0 0 ())
        (draw-box 4 4 62 27)))))
  (println "into repl"))

;(define do-loop 1)
;(define y 1)
;(while (= do-loop 1)
;  (let key (read-character)
;    (if (= key 113)
;      (setq do-loop 0))
;    (prn (cur-xy 5 y))
;    (prn "    ")
;    (prn (cur-xy 5 y))
;    (prn (number->string key)))
;    (setq y (+ y 1)))

;(sleep 5)
;(define i 0)
;(while (not (= i 20))
;     (prn (cur-xy 0 i))
;     (prn "Testing VT100 escape sequences")
;     (sleep 1)
;     (define i (+ i 1)))
