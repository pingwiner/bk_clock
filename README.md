<h1>BK clock project</h1>

<h2>Подключение часов реального времени к БК 0010/0011/0011М</h2>

<a href="schematic.png">Схема</a>

<h3>Подключение библиотеки:</h3>
<pre>
.include "rtc.mac"
</pre>

<h3>Примеры использования:</h3>

<h4>Получение времени в виде форматной строки:</h4>
	
<pre>
timeString: .ASCIZ "d.m.Y, H:i:s, D"  	; строка формата<br>
buffer: .BLKB 64			; буфер для результата<br>

mov #timeString, r0
mov #buffer, r1
call getDateTimeAsString
mov #buffer, r0
emt 64 	
</pre>


<h4>Получение времени:</h4>

<pre>
call getTime

результат в регистрах:
r0 - часы
r1 - минуты
r2 - секунды
</pre>

<h4>Получение даты:</h4>

<pre>
call getDate

результат в регистрах:
r0 - день
r1 - месяц
r2 - год
r3 - день недели (0 - воскресенье, 1 - понедельник ...)
</pre>

<h4>Установка времени</h4>
<pre>
mov #hours, r0 
mov #minutes, r1, 
mov #seconds, r2 
call setTime
</pre>

<h4>Установка даты</h4>
<pre>
mov #day, r0 
mov #month, r1
mov #year, r2 
mov #dayOfWeek, r3
call setDate
</pre>



