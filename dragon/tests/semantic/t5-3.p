(* ERROR: function passed wrong number/type of arguments *)
program main( input, output );
  var b: integer;
  var y: real;

  function foo(a: integer; x: real): integer;
  begin
    foo := a
  end;
begin
  b := foo(y,b) + foo(b,y,10)
end.

