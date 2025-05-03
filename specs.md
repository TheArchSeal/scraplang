# ScrapMechanicLanguage

## Types

* type
* part
* wire

* i8
* i16
* i32
* i64
* u8
* u16
* u32
* u64

* T\*
* T[]
* [T, ...]

## Function

```txt
fun x<T: type>(a: i32, b: u8[], ...c: T): T {

}

primitive wire Bit<> {

}

wire Byte<n: int = 8> :: Bit[8];

wire Test :: Bit, Bit, Byte, Cout = Bit

primitive part And<n: int = 2> :: Bit[n] -> Bit {
}

part HA :: Bit[2] -> Bit, Cout = Bit {

}

```
