; Get RTL_USER_PROCESS_PARAMETERS structure form gs segment
; without __readgsqword predefined function for 64bit only

.code

UserProcessParameter PROC
mov rax, QWORD PTR gs:[30h]     ; TEB
mov rax, [rax+60h]              ; PEB
mov rax, [rax+20h]              ; RTL_USER_PROCESS_PARAMETERS
ret
UserProcessParameter ENDP

end
