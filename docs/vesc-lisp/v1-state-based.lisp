;; VESC LispBM — v1 (original, state-based)
;; M5Stack sent state (0-6) over CAN, Lisp mapped to fixed currents.
;; No ramp, no watchdog, no speed limit.
;; Replaced by v2 (direct current control) around 2026-05.

(define rate 100)      ; Update rate in Hz
(define #motor-state 0) ; Initial motor state

; Initialize state variable
(def state 0)

; Handler for EID CAN-frames
(defun proc-eid (id data)
    (if (= id 0x00000101u32) ; Check if the incoming CAN ID is 0x00000101
        (progn
            (setq state (bufget-i8 data 0)) ; Directly read D0 as int8
            (setvar '#motor-state state) ; Update motor-state variable
            (free data) ; Free the data to prevent memory leaks
        )
    )
)

; Event handler to continuously process incoming CAN messages
(defun event-handler ()
    (loopwhile t
        (recv
            ((event-can-eid (? id) . (? data)) (proc-eid id data))
            (_ nil)
        )
    )
)

; Register and enable the event handler
(event-register-handler (spawn 150 event-handler))
(event-enable 'event-can-eid)

; Handle motor state function
(defun handle-state (state)
  (cond
    ((= state -2) (set-current 0))
    ((= state -1) (set-current 0))
    ((= state 0) (set-current 0))
    ((= state 1) (set-current 10))
    ((= state 2) (set-current 20))
    ((= state 3) (set-current 30))
    ((= state 4) (set-current 40))
    ((= state 5) (set-current 50))
    (t (print "Invalid state"))
  )
)

; Main loop
(loopwhile t
  (progn
    ; Get data
    (define getdist (get-dist 1))
    (define getbat (get-batt))
    (define gettemp (get-temp-mot))
    (define getcurrant (get-current 1))

    ; Format the values with 3 decimal places
    (define formatted-dist (str-from-n getdist "%.3f"))
    (define formatted-bat (str-from-n getbat "%.3f"))
    (define formatted-temp (str-from-n gettemp "%.3f"))
    (define formatted-currant (str-from-n getcurrant "%.3f"))

    ; Create buffers to store the integer values
    (define distance-buffer (bufcreate 4)) ; Buffer for distance
    (define current-buffer (bufcreate 4))  ; Buffer for current
    (define temp-buffer (bufcreate 4))     ; Buffer for temp

    ; Store the integer values in their respective buffers
    (bufset-i32 distance-buffer 0 (to-i getdist))
    (bufset-i32 current-buffer 0 (to-i getcurrant))
    (bufset-i32 temp-buffer 0 (to-i gettemp))

    ; Send separate CAN messages with different IDs
    (can-send-sid 7 distance-buffer) ; Send distance with ID 7
    (can-send-sid 8 current-buffer)  ; Send current with ID 8
    (can-send-sid 9 temp-buffer)     ; Send temp with ID 9

    ; Handle motor state
    (handle-state #motor-state)

    (sleep 0.1)
  )
)
