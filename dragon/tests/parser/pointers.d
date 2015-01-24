program example();
var x, y: integer;
var a, b, c: ^integer;
begin
    a^ := x;
    b := a;
    c := @y
end.
