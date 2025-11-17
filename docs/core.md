Core
====

## System & Language Information
1. [the language version](#the-language-version)
2. [the language major version](#the-language-major-version)
3. [the language minor version](#the-language-minor-version)
4. [the language patch version](#the-language-patch-version)

## Program Control
5. [quit](#quit)
6. [quit with {}](#quit-with-)
7. [the error](#the-error)
8. [error with {}](#error-with-)

## Input/Output (System Module)
9. [print {}](#print-)
10. [print error {}](#print-error-)
11. [write {}](#write-)
12. [write error {}](#write-error-)
13. [read (a) word](#read-a-word)
14. [read (a) line](#read-a-line)
15. [read (a) character](#read-a-character)

## Value Operations
16. [get {}](#get-)
17. [(the) type name (of) {}](#the-type-name-of-)
18. [(a) copy (of) {}](#a-copy-of-)
19. [(the) description (of) {}](#the-description-of-)
20. [(the) debug description (of) {}](#the-debug-description-of-)
21. [(the) hash value (of) {}](#the-hash-value-of-)

## Type Checking & Conversion
22. [{} as (a/an) int/integer](#-as-aan-intinteger)
23. [{} as (a/an) num/number](#-as-aan-numnumber)
24. [{} as (a/an) str/string](#-as-aan-strstring)
25. [{} is (a/an) int/integer](#-is-aan-intinteger)
26. [{} is (a/an) num/number](#-is-aan-numnumber)
27. [{} is (a/an) str/string](#-is-aan-strstring)
28. [{} is (a/an) list](#-is-aan-list)
29. [{} is (a/an) dict/dictionary](#-is-aan-dictdictionary)

## Comparison Operations
30. [{} is {}](#-is-)
31. [{} is not {}](#-is-not-)
32. [{} contains {}](#-contains-)
33. [{} is in {}](#-is-in-)
34. [{} starts with {}](#-starts-with-)
35. [{} ends with {}](#-ends-with-)

## Container Creation
36. [an empty str/string](#an-empty-strstring)
37. [an empty list](#an-empty-list)
38. [an empty dict/dictionary](#an-empty-dictdictionary)

## Container Size & Access
39. [(the) size of {}](#the-size-of-)
40. [item {} in {}](#item--in-)
41. [(the) first item (in/of) {}](#the-first-item-inof-)
42. [(the) mid/middle item (in/of) {}](#the-middle-item-in-)
43. [(the) last item (in/of) {}](#the-last-item-in-)
44. [(the) number of items (in/of) {}](#the-number-of-items-in-)
45. [items {} to {} (in/of) {}](#items--to--in-)

## String Parsing & Item Operations
46. [item {} in/of {} using delimiter {}](#item--inof--using-delimiter-)
47. [(all) items in/of {}](#all-items-inof-)
48. [(all) items in/of {} using delimiter {}](#all-items-inof--using-delimiter-)
49. [items {} to {} in/of {} using delimiter {}](#items--to--inof--using-delimiter-)

## List Operations
50. [insert {} at (the) beginning of {}](#insert--at-the-beginning-of-)
51. [insert {} at (the) end of {}](#insert--at-the-end-of-)
52. [insert {} at index {} into {}](#insert--at-index--into-)
53. [push {} onto {}](#push--onto-)
54. [pop from {}](#pop-from-)
55. [remove (the) first item from {}](#remove-the-first-item-from-)
56. [remove (the) last item from {}](#remove-the-last-item-from-)
57. [remove item {} from {}](#remove-item--from-)
58. [remove items {} to {} from {}](#remove-items--to--from-)
59. [sort {}](#sort-)
60. [reverse {}](#reverse-)
61. [reversed {}](#reversed-)
62. [shuffle {}](#shuffle-)
63. [shuffled {}](#shuffled-)
64. [any item (in/of) {}](#any-item-in-)

## Dictionary Operations
65. [(the) keys (of) {}](#the-keys-of-)
66. [(the) values (of) {}](#the-values-of-)
67. [insert item {} with key {} into {}](#insert-item--with-key--into-)

## String Operations
68. [(the) (first) offset of {} in {}](#the-first-offset-of--in-)
69. [(the) last offset of {} in {}](#the-last-offset-of--in-)
70. [replace all {} with {} in {}](#replace-all--with--in-)
71. [replace first {} with {} in {}](#replace-first--with--in-)
72. [replace last {} with {} in {}](#replace-last--with--in-)
73. [remove all {} from {}](#remove-all--from-)
74. [remove first {} from {}](#remove-first--from-)
75. [remove last {} from {}](#remove-last--from-)
76. [join {}](#join-)
77. [join {} using {}](#join--using-)
78. [format string {} with {}](#format-string--with-)

## Character/Word/Line Operations
79. [char/character {} in/of {}](#character--in-)
80. [word {} in/of {}](#word--in-)
81. [line {} in/of {}](#line--in-)
82. [chars/characters {} to {} in/of {}](#characters--to--in-)
83. [words {} to {} in/of {}](#words--to--in-)
84. [lines {} to {} in/of {}](#lines--to--in-)
85. [(the) list of chars/characters (in/of) {}](#the-list-of-characters-in-)
86. [(the) list of words (in/of) {}](#the-list-of-words-in-)
87. [(the) list of lines (in/of) {}](#the-list-of-lines-in-)
88. [(the) number of chars/characters (in/of) {}](#the-number-of-characters-in-)
89. [(the) number of words (in/of) {}](#the-number-of-words-in-)
90. [(the) number of lines (in/of) {}](#the-number-of-lines-in-)
91. [insert {} at char/character {} in {}](#insert--at-character--in-)
92. [replace char/character {} with {} in {}](#replace-character--with--in-)
93. [replace word {} with {} in {}](#replace-word--with--in-)
94. [replace line {} with {} in {}](#replace-line--with--in-)
95. [replace chars/characters {} to {} with {} in {}](#replace-charscharacters--to--with--in-)
96. [replace words {} to {} with {} in {}](#replace-words--to--with--in-)
97. [replace lines {} to {} with {} in {}](#replace-lines--to--with--in-)
98. [remove char/character {} from {}](#remove-character--from-)
99. [remove word {} from {}](#remove-word--from-)
100. [remove line {} from {}](#remove-line--from-)
101. [remove chars/characters {} to {} from {}](#remove-characters--to--from-)
102. [remove words {} to {} from {}](#remove-words--to--from-)
103. [remove lines {} to {} from {}](#remove-lines--to--from-)
104. [any char/character in/of {}](#any-character-in-)
105. [any word in/of {}](#any-word-in-)
106. [any line in/of {}](#any-line-in-)
107. [(the) mid/middle char/character in/of {}](#the-middle-character-in-)
108. [(the) mid/middle word in/of {}](#the-middle-word-in-)
109. [(the) mid/middle line in/of {}](#the-middle-line-in-)
110. [(the) last char/character in/of {}](#the-last-character-in-)
111. [(the) last word in/of {}](#the-last-word-in-)
112. [(the) last line in/of {}](#the-last-line-in-)

## Character Encoding
113. [(the) char/character (of) {}](#the-charcharacter-of-)
114. [(the) numToChar (of) {}](#the-numtochar-of-)
115. [(the) ord/ordinal (of) {}](#the-ordinal-of-)
116. [(the) charToNum (of) {}](#the-chartonum-of-)

## Range Operations
117. [{} up to {}](#-up-to-)
118. [(the) lower bound (in/of) {}](#the-lower-bound-of-)
119. [(the) upper bound (in/of) {}](#the-upper-bound-of-)
120. [{} is closed](#-is-closed)
121. [{} overlaps (with) {}](#-overlaps-with-)
122. [(a) random number (in/of) {}](#a-random-number-in-)

## Mathematical Functions
123. [(the) abs (of) {}](#the-abs-of-)
124. [(the) sin (of) {}](#the-sin-of-)
125. [(the) asin (of) {}](#the-asin-of-)
126. [(the) cos (of) {}](#the-cos-of-)
127. [(the) acos (of) {}](#the-acos-of-)
128. [(the) tan (of) {}](#the-tan-of-)
129. [(the) atan (of) {}](#the-atan-of-)
130. [(the) exp (of) {}](#the-exp-of-)
131. [(the) exp2 (of) {}](#the-exp2-of-)
132. [(the) expm1 (of) {}](#the-expm1-of-)
133. [(the) log2 (of) {}](#the-log2-of-)
134. [(the) log10 (of) {}](#the-log10-of-)
135. [(the) log (of) {}](#the-log-of-)
136. [(the) sqrt (of) {}](#the-sqrt-of-)
137. [(the) square root (of) {}](#the-square-root-of-)
138. [(the) ceil (of) {}](#the-ceil-of-)
139. [(the) floor (of) {}](#the-floor-of-)
140. [round {}](#round-)
141. [trunc/truncate {}](#truncate-)
142. [(the) max/maximum (value) (of) {}](#the-maximum-value-of-)
143. [(the) min/minimum (value) (of) {}](#the-minimum-value-of-)
144. [(the) avg/average (value) (of) {}](#the-average-value-of-)

---

the language version
--------------------

### Usage

    the language version

### Description
Returns the current version of the Sif language as a string.


the language major version
--------------------------

### Usage

    the language major version

### Description
Returns the current major version of the Sif language as an integer.


the language minor version
--------------------------

### Usage

    the language minor version

### Description
Returns the current minor version of the Sif language as an integer.


the language patch version
--------------------------

### Usage

    the language patch version

### Description
Returns the current patch version of the Sif language as an integer.


quit
----

### Usage

    quit

### Description
Quit the program successfully.


quit with {}
------------

### Usage

    quit with errorCode

**errorCode** must be an integer.

### Description
Quit the program using **errorCode** as the status code.


the error
---------

### Usage

    the error

### Description
Returns the last error that occurred, or empty if no error occurred.


error with {}
-------------

### Usage

    error with message

**message** may be anything.

### Description
Raises an error with the given **message**.


print {}
--------

### Usage

    print value

**value** may be anything.

### Description
Prints a value to stdout, followed by a new line character.


print error {}
--------------

### Usage

    print error value

**value** may be anything.

### Description
Print a value to stderr, followed by a new line character.


write {}
--------

### Usage

    write value

**value** may be anything.

### Description
Write a value to stdout.


write error {}
--------------

### Usage

    write error value

**value** may be anything.

### Description
Write a value to stderr.


get {}
------

### Usage

    get value

**value** may be anything.

### Examples

    get the language version
    print it

### Description
Evaluates to **value**. This function has no effect on the value it returns, but may be used to make statements read more naturally.


(the) type name (of) {}
-----------------------

### Usage

    the type name of value

**value** may be anything.

### Description
This function evaluates to the type name of **value** as a string.


(a) copy (of) {}
----------------

### Usage

    a copy of value

**value** may be anything.

### Examples

    a copy of [1, 2, 3]
    a copy of "Hello, World!"

### Description
Evaluates to a copy of **value**.


(the) description (of) {}
-------------------------

### Usage

    the description of value

**value** may be anything.

### Examples

    the description of 1
    the description of [1, 2, 3]
    the description of ["one": 1, "two": 2]

### Description
Evaluates to a string describing **value**.


(the) debug description (of) {}
-------------------------------

### Usage

    the debug description of value

**value** may be anything.

### Description
Evaluates to a detailed debug string describing **value**.


(the) hash value (of) {}
------------------------

### Usage

    the hash value of value

**value** may be anything.

### Examples

    the hash value of 42
    the hash value of "Hello, World!"
    the hash value of [1, 2, 3]

### Description
Evaluates to an integer representing a hash of **value**.


{} as (a/an) int/integer
------------------------

### Usage

    value as an integer

**value** may be a string or number.

### Description
Converts **value** to an integer. Strings must contain valid numeric representations.


{} as (a/an) num/number
-----------------------

### Usage

    value as a number

**value** may be a string or integer.

### Description
Converts **value** to a floating-point number.


{} as (a/an) str/string
-----------------------

### Usage

    value as a string

**value** may be anything.

### Description
Converts **value** to its string representation.


{} is (a/an) int/integer
------------------------

### Usage

    value is an integer

**value** may be anything.

### Description
Returns true if **value** is an integer, false otherwise.


{} is (a/an) num/number
-----------------------

### Usage

    value is a number

**value** may be anything.

### Description
Returns true if **value** is a number (integer or float), false otherwise.


{} is (a/an) str/string
-----------------------

### Usage

    value is a string

**value** may be anything.

### Description
Returns true if **value** is a string, false otherwise.


{} is (a/an) list
-----------------

### Usage

    value is a list

**value** may be anything.

### Description
Returns true if **value** is a list, false otherwise.


{} is (a/an) dict/dictionary
----------------------------

### Usage

    value is a dictionary

**value** may be anything.

### Description
Returns true if **value** is a dictionary, false otherwise.


{} is {}
--------

### Usage

    value1 is value2

**value1** and **value2** may be anything.

### Description
Returns true if **value1** equals **value2**. Special handling for empty containers.


{} is not {}
------------

### Usage

    value1 is not value2

**value1** and **value2** may be anything.

### Description
Returns true if **value1** does not equal **value2**.


{} contains {}
--------------

### Usage

    container contains item

**container** may be a string, list, dictionary, or range.
**item** may be anything.

### Description
Returns true if **container** contains **item**.


{} is in {}
-----------

### Usage

    item is in container

**item** may be anything.
**container** may be a string, list, dictionary, or range.

### Description
Returns true if **item** is contained in **container**.


{} starts with {}
-----------------

### Usage

    string starts with prefix

**string** and **prefix** must be strings.

### Description
Returns true if **string** starts with **prefix**.


{} ends with {}
---------------

### Usage

    string ends with suffix

**string** and **suffix** must be strings.

### Description
Returns true if **string** ends with **suffix**.


an empty str/string
-------------------

### Usage

    an empty string

### Description
Returns an empty string.


an empty list
-------------

### Usage

    an empty list

### Description
Returns an empty list.


an empty dict/dictionary
------------------------

### Usage

    an empty dictionary

### Description
Returns an empty dictionary.


(the) size of {}
----------------

### Usage

    the size of container

**container** may be a string, list, dictionary, or range.

### Description
Returns the number of items in **container**.


item {} in {}
-------------

### Usage

    item index in container

**index** must be an integer.
**container** may be a list or dictionary.

### Description
Returns the item at **index** in **container**.


(the) first item (in/of) {}
---------------------------

### Usage

    the first item in container

**container** must be a list.

### Description
Returns the first item in **container**. The list must not be empty.


(the) mid/middle item (in/of) {}
--------------------------------

### Usage

    the middle item in container

**container** must be a list.

### Description
Returns the middle item in **container**. For even-length lists, returns the first of the two middle items.


(the) last item (in/of) {}
--------------------------

### Usage

    the last item in container

**container** must be a list.

### Description
Returns the last item in **container**. The list must not be empty.


(the) number of items (in/of) {}
--------------------------------

### Usage

    the number of items in container

**container** may be a string, list, dictionary, or range.

### Description
Returns the number of items in **container** as an integer.


items {} to {} (in/of) {}
-------------------------

### Usage

    items start to end in container

**start** and **end** must be integers.
**container** must be a list.

### Description
Returns a slice of **container** from **start** to **end** (inclusive).


item {} in/of {} using delimiter {}
------------------------------------

### Usage

    item index in/of string using delimiter delimiter

**index** must be an integer.
**string** must be a string.
**delimiter** must be a string.

### Description
Returns the item at **index** when **string** is split by **delimiter**. Similar to `item {} in {}` but uses a custom delimiter instead of the default comma.


(all) items in/of {}
--------------------

### Usage

    items in/of string

**string** must be a string.

### Description
Splits **string** by commas and returns a list of all items. This is the string parsing operation inspired by HyperTalk. Empty strings return an empty list, and trailing delimiters create an empty string item at the end.


(all) items in/of {} using delimiter {}
----------------------------------------

### Usage

    items in/of string using delimiter delimiter

**string** must be a string.
**delimiter** must be a string.

### Description
Splits **string** by **delimiter** and returns a list of all items. Supports multi-character delimiters. Empty strings return an empty list, and trailing delimiters create an empty string item at the end.


items {} to {} in/of {} using delimiter {}
-------------------------------------------

### Usage

    items start to end in/of string using delimiter delimiter

**start** and **end** must be integers.
**string** must be a string.
**delimiter** must be a string.

### Description
Returns a list of items from **start** to **end** (inclusive) when **string** is split by **delimiter**.


insert {} at (the) beginning of {}
----------------------------------

### Usage

    insert item at the beginning of list

**item** may be anything.
**list** must be a list.

### Description
Inserts **item** at the beginning of **list**. Modifies the list in place.


insert {} at (the) end of {}
----------------------------

### Usage

    insert item at the end of list

**item** may be anything.
**list** must be a list.

### Description
Inserts **item** at the end of **list**. Modifies the list in place.


insert {} at index {} into {}
-----------------------------

### Usage

    insert item at index position into list

**item** may be anything.
**position** must be an integer.
**list** must be a list.

### Description
Inserts **item** at **position** in **list**. Modifies the list in place.


push {} onto {}
---------------

### Usage

    push item onto list

**item** may be anything.
**list** must be a list.

### Description
Pushes **item** onto the end of **list**. Modifies the list in place and returns the list. This is a stack operation equivalent to `insert {} at (the) end of {}`.


pop from {}
-----------

### Usage

    pop from list

**list** must be a list.

### Description
Pops and returns the last item from **list**. The list must not be empty. This is a stack operation that removes and returns the last element. Returns empty if the list is empty.


remove (the) first item from {}
-------------------------------

### Usage

    remove the first item from list

**list** must be a list.

### Description
Removes and returns the first item from **list**. The list must not be empty.


remove (the) last item from {}
------------------------------

### Usage

    remove the last item from list

**list** must be a list.

### Description
Removes and returns the last item from **list**. The list must not be empty.


remove item {} from {}
----------------------

### Usage

    remove item index from list

**index** must be an integer.
**list** must be a list.

### Description
Removes and returns the item at **index** from **list**.


remove items {} to {} from {}
-----------------------------

### Usage

    remove items start to end from list

**start** and **end** must be integers.
**list** must be a list.

### Description
Removes items from **start** to **end** (inclusive) from **list**.


sort {}
-------

### Usage

    sort container

**container** must be a list.

### Description
Sorts **container** in place. Works with numbers and strings (case-insensitive).


reverse {}
----------

### Usage

    reverse container

**container** must be a list.

### Description
Reverses **container** in place.


reversed {}
-----------

### Usage

    reversed container

**container** must be a list.

### Description
Returns a new list with the items of **container** in reverse order.


shuffle {}
----------

### Usage

    shuffle list

**list** must be a list.

### Description
Randomly shuffles **list** in place.


shuffled {}
-----------

### Usage

    shuffled list

**list** must be a list.

### Description
Returns a new list with the items of **list** in random order.


any item (in/of) {}
-------------------

### Usage

    any item in container

**container** must be a list.

### Description
Returns a random item from **container**.


(the) keys (of) {}
------------------

### Usage

    the keys of dictionary

**dictionary** must be a dictionary.

### Description
Returns a list containing all keys from **dictionary**.


(the) values (of) {}
--------------------

### Usage

    the values of dictionary

**dictionary** must be a dictionary.

### Description
Returns a list containing all values from **dictionary**.


insert item {} with key {} into {}
----------------------------------

### Usage

    insert item value with key name into dictionary

**value** may be anything.
**name** may be anything.
**dictionary** must be a dictionary.

### Description
Inserts **value** with **name** as the key into **dictionary**.


(the) (first) offset of {} in {}
--------------------------------

### Usage

    the first offset of substring in string

**substring** and **string** must be strings.

### Description
Returns the first position where **substring** occurs in **string**, or -1 if not found.


(the) last offset of {} in {}
-----------------------------

### Usage

    the last offset of substring in string

**substring** and **string** must be strings.

### Description
Returns the last position where **substring** occurs in **string**, or -1 if not found.


replace all {} with {} in {}
----------------------------

### Usage

    replace all old with new in string

**old**, **new**, and **string** must be strings or lists.

### Description
Replaces all occurrences of **old** with **new** in **string**.


replace first {} with {} in {}
------------------------------

### Usage

    replace first old with new in string

**old**, **new**, and **string** must be strings.

### Description
Replaces the first occurrence of **old** with **new** in **string**.


replace last {} with {} in {}
-----------------------------

### Usage

    replace last old with new in string

**old**, **new**, and **string** must be strings.

### Description
Replaces the last occurrence of **old** with **new** in **string**.


remove all {} from {}
---------------------

### Usage

    remove all substring from string

**substring** and **string** must be strings.

### Description
Removes all occurrences of **substring** from **string**.


remove first {} from {}
-----------------------

### Usage

    remove first substring from string

**substring** and **string** must be strings.

### Description
Removes the first occurrence of **substring** from **string**.


remove last {} from {}
----------------------

### Usage

    remove last substring from string

**substring** and **string** must be strings.

### Description
Removes the last occurrence of **substring** from **string**.


join {}
-------

### Usage

    join list

**list** must be a list of strings.

### Description
Joins all strings in **list** into a single string.


join {} using {}
----------------

### Usage

    join list using separator

**list** must be a list of strings.
**separator** must be a string.

### Description
Joins all strings in **list** using **separator** between them.


format string {} with {}
------------------------

### Usage

    format string template with arguments

**template** must be a string.
**arguments** must be a list.

### Description
Formats **template** string by replacing placeholders with values from **arguments**.


char/character {} in/of {}
--------------------------

### Usage

    character index in string

**index** must be an integer.
**string** must be a string.

### Description
Returns the character at **index** in **string**.


word {} in/of {}
----------------

### Usage

    word index in string

**index** must be an integer.
**string** must be a string.

### Description
Returns the word at **index** in **string**.


line {} in/of {}
----------------

### Usage

    line index in string

**index** must be an integer.
**string** must be a string.

### Description
Returns the line at **index** in **string**.


chars/characters {} to {} in/of {}
----------------------------------

### Usage

    characters start to end in string

**start** and **end** must be integers.
**string** must be a string.

### Description
Returns characters from **start** to **end** in **string**.


words {} to {} in/of {}
-----------------------

### Usage

    words start to end in string

**start** and **end** must be integers.
**string** must be a string.

### Description
Returns words from **start** to **end** in **string**.


lines {} to {} in/of {}
-----------------------

### Usage

    lines start to end in string

**start** and **end** must be integers.
**string** must be a string.

### Description
Returns lines from **start** to **end** in **string**.


(the) list of chars/characters (in/of) {}
------------------------------------------

### Usage

    the list of characters in string

**string** must be a string.

### Description
Returns a list containing all characters in **string**.


(the) list of words (in/of) {}
------------------------------

### Usage

    the list of words in string

**string** must be a string.

### Description
Returns a list containing all words in **string**.


(the) list of lines (in/of) {}
------------------------------

### Usage

    the list of lines in string

**string** must be a string.

### Description
Returns a list containing all lines in **string**.


(the) number of chars/characters (in/of) {}
-------------------------------------------

### Usage

    the number of characters in string

**string** must be a string.

### Description
Returns the number of characters in **string**.


(the) number of words (in/of) {}
--------------------------------

### Usage

    the number of words in string

**string** must be a string.

### Description
Returns the number of words in **string**.


(the) number of lines (in/of) {}
--------------------------------

### Usage

    the number of lines in string

**string** must be a string.

### Description
Returns the number of lines in **string**.


insert {} at char/character {} in {}
------------------------------------

### Usage

    insert text at character position in string

**text** must be a string.
**position** must be an integer.
**string** must be a string.

### Description
Inserts **text** at character **position** in **string**.


replace char/character {} with {} in {}
---------------------------------------

### Usage

    replace character index with text in string

**index** must be an integer.
**text** must be a string.
**string** must be a string.

### Description
Replaces the character at **index** with **text** in **string**.


replace word {} with {} in {}
-----------------------------

### Usage

    replace word index with text in string

**index** must be an integer.
**text** must be a string.
**string** must be a string.

### Description
Replaces the word at **index** with **text** in **string**.


replace line {} with {} in {}
-----------------------------

### Usage

    replace line index with text in string

**index** must be an integer.
**text** must be a string.
**string** must be a string.

### Description
Replaces the line at **index** with **text** in **string**.


replace chars/characters {} to {} with {} in {}
-----------------------------------------------

### Usage

    replace characters start to end with text in string

**start** and **end** must be integers.
**text** must be a string.
**string** must be a string.

### Description
Replaces characters from **start** to **end** with **text** in **string**.


replace words {} to {} with {} in {}
------------------------------------

### Usage

    replace words start to end with text in string

**start** and **end** must be integers.
**text** must be a string.
**string** must be a string.

### Description
Replaces words from **start** to **end** with **text** in **string**.


replace lines {} to {} with {} in {}
------------------------------------

### Usage

    replace lines start to end with text in string

**start** and **end** must be integers.
**text** must be a string.
**string** must be a string.

### Description
Replaces lines from **start** to **end** with **text** in **string**.


remove char/character {} from {}
--------------------------------

### Usage

    remove character index from string

**index** must be an integer.
**string** must be a string.

### Description
Removes the character at **index** from **string**.


remove word {} from {}
----------------------

### Usage

    remove word index from string

**index** must be an integer.
**string** must be a string.

### Description
Removes the word at **index** from **string**.


remove line {} from {}
----------------------

### Usage

    remove line index from string

**index** must be an integer.
**string** must be a string.

### Description
Removes the line at **index** from **string**.


remove chars/characters {} to {} from {}
----------------------------------------

### Usage

    remove characters start to end from string

**start** and **end** must be integers.
**string** must be a string.

### Description
Removes characters from **start** to **end** from **string**.


remove words {} to {} from {}
-----------------------------

### Usage

    remove words start to end from string

**start** and **end** must be integers.
**string** must be a string.

### Description
Removes words from **start** to **end** from **string**.


remove lines {} to {} from {}
-----------------------------

### Usage

    remove lines start to end from string

**start** and **end** must be integers.
**string** must be a string.

### Description
Removes lines from **start** to **end** from **string**.


any char/character in/of {}
---------------------------

### Usage

    any character in string

**string** must be a string.

### Description
Returns a random character from **string**.


any word in/of {}
-----------------

### Usage

    any word in string

**string** must be a string.

### Description
Returns a random word from **string**.


any line in/of {}
-----------------

### Usage

    any line in string

**string** must be a string.

### Description
Returns a random line from **string**.


(the) mid/middle char/character in/of {}
----------------------------------------

### Usage

    the middle character in string

**string** must be a string.

### Description
Returns the middle character in **string**.


(the) mid/middle word in/of {}
------------------------------

### Usage

    the middle word in string

**string** must be a string.

### Description
Returns the middle word in **string**.


(the) mid/middle line in/of {}
------------------------------

### Usage

    the middle line in string

**string** must be a string.

### Description
Returns the middle line in **string**.


(the) last char/character in/of {}
----------------------------------

### Usage

    the last character in string

**string** must be a string.

### Description
Returns the last character in **string**.


(the) last word in/of {}
------------------------

### Usage

    the last word in string

**string** must be a string.

### Description
Returns the last word in **string**.


(the) last line in/of {}
------------------------

### Usage

    the last line in string

**string** must be a string.

### Description
Returns the last line in **string**.


(the) char/character (of) {}
----------------------------

### Usage

    the character of number

**number** must be an integer.

### Description
Returns the Unicode character corresponding to **number**.


(the) numToChar (of) {}
-----------------------

### Usage

    the numToChar of number

**number** must be an integer.

### Description
Returns the Unicode character corresponding to **number**. Alias for "the character of".


(the) ord/ordinal (of) {}
-------------------------

### Usage

    the ordinal of character

**character** must be a single-character string.

### Description
Returns the Unicode code point of **character**.


(the) charToNum (of) {}
-----------------------

### Usage

    the charToNum of character

**character** must be a single-character string.

### Description
Returns the Unicode code point of **character**. Alias for "the ordinal of".


{} up to {}
-----------

### Usage

    start up to end

**start** and **end** must be integers.

### Description
Creates a range from **start** to **end** (inclusive).


(the) lower bound (in/of) {}
----------------------------

### Usage

    the lower bound of range

**range** must be a range.

### Description
Returns the lower bound of **range**.


(the) upper bound (in/of) {}
----------------------------

### Usage

    the upper bound of range

**range** must be a range.

### Description
Returns the upper bound of **range**.


{} is closed
------------

### Usage

    range is closed

**range** must be a range.

### Description
Returns true if **range** is closed (includes both endpoints).


{} overlaps (with) {}
---------------------

### Usage

    range1 overlaps with range2

**range1** and **range2** must be ranges.

### Description
Returns true if **range1** and **range2** overlap.


(a) random number (in/of) {}
----------------------------

### Usage

    a random number in range

**range** must be a range.

### Description
Returns a random number within **range**.


(the) abs (of) {}
-----------------

### Usage

    the abs of number

**number** must be a number.

### Description
Returns the absolute value of **number**.


(the) sin (of) {}
-----------------

### Usage

    the sin of number

**number** must be a number.

### Description
Returns the sine of **number** (in radians).


(the) asin (of) {}
------------------

### Usage

    the asin of number

**number** must be a number.

### Description
Returns the arcsine of **number**.


(the) cos (of) {}
-----------------

### Usage

    the cos of number

**number** must be a number.

### Description
Returns the cosine of **number** (in radians).


(the) acos (of) {}
------------------

### Usage

    the acos of number

**number** must be a number.

### Description
Returns the arccosine of **number**.


(the) tan (of) {}
-----------------

### Usage

    the tan of number

**number** must be a number.

### Description
Returns the tangent of **number** (in radians).


(the) atan (of) {}
------------------

### Usage

    the atan of number

**number** must be a number.

### Description
Returns the arctangent of **number**.


(the) exp (of) {}
-----------------

### Usage

    the exp of number

**number** must be a number.

### Description
Returns e raised to the power of **number**.


(the) exp2 (of) {}
------------------

### Usage

    the exp2 of number

**number** must be a number.

### Description
Returns 2 raised to the power of **number**.


(the) expm1 (of) {}
-------------------

### Usage

    the expm1 of number

**number** must be a number.

### Description
Returns e raised to the power of **number**, minus 1.


(the) log2 (of) {}
------------------

### Usage

    the log2 of number

**number** must be a positive number.

### Description
Returns the base-2 logarithm of **number**.


(the) log10 (of) {}
-------------------

### Usage

    the log10 of number

**number** must be a positive number.

### Description
Returns the base-10 logarithm of **number**.


(the) log (of) {}
-----------------

### Usage

    the log of number

**number** must be a positive number.

### Description
Returns the natural logarithm of **number**.


(the) sqrt (of) {}
------------------

### Usage

    the sqrt of number

**number** must be a non-negative number.

### Description
Returns the square root of **number**.


(the) square root (of) {}
-------------------------

### Usage

    the square root of number

**number** must be a non-negative number.

### Description
Returns the square root of **number**. Alias for "the sqrt of".


(the) ceil (of) {}
------------------

### Usage

    the ceil of number

**number** must be a number.

### Description
Returns **number** rounded up to the nearest integer.


(the) floor (of) {}
-------------------

### Usage

    the floor of number

**number** must be a number.

### Description
Returns **number** rounded down to the nearest integer.


round {}
--------

### Usage

    round number

**number** must be a number.

### Description
Returns **number** rounded to the nearest integer.


trunc/truncate {}
-----------------

### Usage

    truncate number

**number** must be a number.

### Description
Returns **number** with the fractional part removed.


(the) max/maximum (value) (of) {}
---------------------------------

### Usage

    the maximum value of list

**list** must be a list of numbers.

### Description
Returns the largest value in **list**.


(the) min/minimum (value) (of) {}
---------------------------------

### Usage

    the minimum value of list

**list** must be a list of numbers.

### Description
Returns the smallest value in **list**.


(the) avg/average (value) (of) {}
---------------------------------

### Usage

    the average value of list

**list** must be a list of numbers.

### Description
Returns the average (mean) of all values in **list**.


read (a) word
-------------

### Usage

    read a word

### Description
Evaluates to a string read from stdin. This command will read one word from stdin, skipping whitespaces. A whitespace is defined as one of the following ASCII characters:
- space (0x20, " ")
- form feed (0x0c, "\f")
- line feed (0x0a, "\n")
- carriage return (0x0d, "\r")
- horizontal tab (0x09, "\t")
- or vertical tab (0x0b, "\v")


read (a) line
-------------

### Usage

    read a line

### Description
Evaluates to a string read from stdin. This command will read one whole line from stdin, stopping once the line feed (0x0a, "\n") character is encountered.


read (a) character
------------------

### Usage

    read a character

### Description
Evaluates to a string read from stdin. This command will read exactly one UTF8-encoded character from stdin.