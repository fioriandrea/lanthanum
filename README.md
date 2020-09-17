# Lanthanum

## Description

Lanthanum is a small, byte code interpreted, garbage collected, Python-like programming language implemented in C.
The language is dynamically typed, has built-in arrays and dictionaries, closures, higher order functions and other language constructs. 
The language is an improved version of [lamp](https://github.com/fioriandrea/lamp), so the languages are quite similar. 

## Installation

### Arch Linux and Arch-based Distros

On Arch-based linux distros, lanthanum can be installed from the AUR as [lanthanum-git](https://aur.archlinux.org/packages/lanthanum-git/).

If yay is installed on your system, lanthanum can be installed by running:

```sh
yay -S lanthanum-git
```

## Project Structure

The code behind Lanthanum is inspired by [Wren code](https://github.com/wren-lang/wren).
The project structure is indeed similar to the one used in Wren, although there are quite a few differences.

## Grammar

**program** -> statement\* EOF  
**statement** -> print | let | if | while | func |  ret | break | continue | expressionStat  
**print** -> 'print' expression NEW_LINE  
**let** -> 'let' IDENTIFIER '=' expression NEW_LINE  
**if** -> 'if' expression block ('elif' expression block)\* ('else' block)?  
**while** -> 'while' expression block  
**func** -> 'func' IDENTIFIER '(' paramList ')' block  
**ret** -> 'ret' (expression)? NEW_LINE  
**break** -> 'break' NEW_LINE  
**continue** -> 'continue' NEW_LINE  
**expressionStat** -> expression NEW_LINE  
**block** -> INDENT statement* DEDENT  

**expression** -> comma   
**comma** -> nonCommaExpr (',' nonCommaExpr)\*  
**nonCommaExpr** -> assign  
**assign** -> ternary '=' expression  
**ternary** -> logicalSum ('?' expression : expression)?  
**logicalSum** -> and (('or' | 'xor') and)\*  
**and** -> equal ('and' equal)\*  
**equal** -> comparison (('==' | '!=') comparison)\*  
**comparison** -> sum (('<' | '<=' | '>' | '>=') sum)\*  
**sum** -> mult (('+' | '-' | '++') mult)\*  
**mult** -> pow (('\*' | '/' | '%') pow)\*  
**pow** -> unary | unary '^' pow  
**unary** -> call | ('-' | '!') unary  
**call** -> functionCall | indexing  
**functionCall** -> primary '(' argList ')'  
**argList** -> nonCommaExpr (',' nonCommaExpr)\* | ''  
**indexing** -> primary '[' expression ']'  
**primary** ->  basic | '(' expression ')' | array | map   
**basic** -> STRING | NUMBER | TRUE | FALSE | NIHL | IDENTIFIER  
**array** -> '[' arrayList ']'  
**arrayList** -> nonCommaExpr (',' nonCommaExpr)\* | ''  
**map** -> '{' mapList '}'  
**mapList** -> mapElement (',' mapElement) | ''  
**mapElement** -> nonCommaExpr '=>' nonCommaExpr  

## Types

Lanthanum has the following built-in types:

### Booleans 

A boolean value can either be true or false. 
When checking a condition, lanthanum considers nihl and false as false, anything else as true.

### Nihl

nihl is a special type used to indicate the absence of any value.
It is treated as false in conditions.

### Numbers

Every number in Lanthanum is a floating point number. 
Internally, numbers are rappresented as a C double.

### Strings

Strings are "" or '' delimited sequences of characters.
Since no escaping is supported inside strings, strings can span several lines.

### Maps

Maps are associative arrays built-in into Lanthanum. 
Internally they are implemented as hash tables. 
Maps support nesting.

### Arrays

Lanthanum arrays are the basic container type. 
They can contain any kind of value and can nest.
Internally they are implemented as dynamic arrays.
Arrays are 0 indexed.

### Functions

Lanthanum functions are first class citizens. 
They can be stored in variables, passed to other functions and returned by other functions.
Lanthanum supports closures.
This means that functions are bound together with their lexical environment.
Any time that a function is created, it is wrapped inside a closure.

## Operators

Lanthanum has the following operators:

### Logical

* or (e.g. true or false)
* xor (e.g. true xor false)
* and (e.g. true and false)
* not (e.g. !true)
* ternary (e.g. true ? false : true)

### Arithmetic

* plus (e.g. 10 + 20)
* minus (e.g. 10 - 20)
* times (e.g. 10 * 3)
* division (e.g. 10 / 3)
* mod (e.g. 10 % 3)
* power (e.g. 10 ^ 3)

### Other

* concatenation (e.g [1, 2, 3] ++ [4, 5, 6]; 'hello' ++ ' world')
* indexing (e.g. array[10]; map["hello"])

## Sample Programs

### Arrays

```
"comments are done with string expressions"
print [1, 2, 'hallo', ['wie gehet es dir?', ['empty']]] ++ ['hi']
print [1, 2, 'hallo', ['wie gehet es dir?', ['empty']]][3]
```

### Maps

```
print {
    'list' => [1, 2, 'hallo', ['wie gehet es dir?', ['empty']]],
    'meat' => true,
    'horse' => true xor true,
    false => 'true'
}

print {
    'list' => [1, 2, 'hallo', ['wie gehet es dir?', ['empty']]],
    'meat' => true,
    'horse' => true xor true,
    false => 'true'
}[false]

print {
    'list' => [1, 2, 'hallo', ['wie gehet es dir?', ['empty']]],
    'meat' => true,
    'horse' => true xor true
}['non present']
```

### Strings

```
print 'guten morgen'[3]
print 'guten' ++ ' morgen'
```

### Expressions

```
print (10 > 2 ? true : nihl) xor false
print (len([1, 2, 3] ++ ['asd', 'qwe']) == 5 and typeof(true) == 'boolean') xor true
```

### Variable Declaration

```
let meat = 'pork' ++ 'cooked'
let map = {
    'bread' => 10,
    'garlic' => 20,
    'vitamin c' => 30,
    'list' => [meat, 'eggs', ['brot']]
}

print map
```

### Assignments

```
let meat = 'pork' ++ 'cooked'
let map = {
    'list' => [meat, 'eggs', ['brot']]
}
map['recursion'] = map
let letter = meat[2]
meat = map['list'][2]

print map
print letter
print meat
```

### Conditional Statements

```
let fizz = 30

if fizz % 15 == 0
    print 'FizzBuzz'
elif fizz % 3 == 0
    print 'Fizz'
elif fizz % 5 == 0
    print 'Buzz'
else
    print fizz
```

### Loops

```
"for loops are not yet supported"

let fizz = 1

while fizz < 100
    if fizz % 15 == 0
        print 'FizzBuzz'
    elif fizz % 3 == 0
        print 'Fizz'
    elif fizz % 5 == 0
        print 'Buzz'
    else
        print fizz
    fizz = fizz + 1
```

### Function Definition

```
func factorial(n)
    if n <= 0
        ret 1
    else
        ret n * factorial(n - 1)

print factorial(20)
```

### Closures

```
func sayHiConstructor()
    let greeting = 'Hi'

    func change(newGreeting)
        greeting = newGreeting

    func sayHi()
        print greeting

    ret {
        'change' => change,
        'sayHi' => sayHi
    }

let hiSayer = sayHiConstructor()
hiSayer['sayHi']()
hiSayer['change']('Hallo')
hiSayer['sayHi']()
```
