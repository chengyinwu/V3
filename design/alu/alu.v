/* 
   Source   : alu/alu.v
   Synopsis : A sample arithmetic logic unit (ALU) design.
   Authors  : Cheng-Yin Wu, NTUGIEE
   Date     : 2013/02/20
   Version  : 1.0
*/


module alu(
   // General I/O Ports
   clk,
   rst,
   // Input Ports
   a,
   inst,
   // Output Ports
   out
);

// General I/O Ports
input          clk;
input          rst;
// Input Ports
input  [7:0]   a;
input  [1:0]   inst;
// Output Ports
output [15:0]  out;

reg [15:0]     out;

always@ (posedge clk) begin
   if (!rst) begin
      out <= 16'd0;
   end
   else begin
      case(inst)
         2'b00 : out <= out + {8'b0, a};
         2'b01 : out <= out - {8'b0, a};
         2'b10 : out <= out * {8'b0, a};
         2'b11 : out <= {8'b0, (out[7:0] & a)};
      endcase
   end
end

endmodule
