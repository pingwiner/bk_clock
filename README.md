BK clock project

Подключение часов реального времени к БК 0010/0011/0011М

	Подключение библиотеки:
.include "rtc.mac"


Примеры использования:

	Получение времени в виде форматной строки:

timeString: .ASCIZ "d.m.Y, H:i:s, D"  	; строка формата
buffer: .BLKB 64			; буфер для результата

mov #timeString, r0
mov #buffer, r1
call getDateTimeAsString
mov #buffer, r0
emt 64 	


	Получение времени:

call getTime

результат в регистрах:
r0 - часы
r1 - минуты
r2 - секунды


	Получение даты:

call getDate

результат в регистрах:
r0 - день
r1 - месяц
r2 - год
r3 - день недели (0 - воскресенье, 1 - понедельник ...)


	Установка времени

mov #hours, r0 
mov #minutes, r1, 
mov #seconds, r2 
call setTime


	Установка даты
mov #day, r0 
mov #month, r1
mov #year, r2 
mov #dayOfWeek, r3
call setDate




