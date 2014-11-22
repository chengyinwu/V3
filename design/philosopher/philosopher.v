/* 
   Source   : philosopher/philosopher.v
   Synopsis : A sample philosopher design for the dinning philosopher problem.
   Authors  : Fabio Somenzi, Colorado. Rewritten by Cheng-Yin Wu, NTUGIEE
   Date     : 2013/02/20
   Version  : 1.0
*/

`define THINKING  2'd0
`define READING   2'd1
`define EATING    2'd2
`define HUNGRY    2'd3

module philosopher(clk, reset, out, left, right, init, random);
input clk, reset, random;
input [1:0] left, right, init;
output [1:0] out;

reg [1:0] state;

assign out = state;

always @(posedge clk) begin
   if (!reset) begin
      state <= init;
   end
   else begin
      case (state)
         `READING : begin
            if (left == `THINKING) begin
               state <= `THINKING;
            end
         end
         `THINKING : begin
            if (right == `READING) begin
               state <= `READING;
            end
            else begin
               state <= random ? `THINKING : `HUNGRY;
            end
         end
         `EATING : begin
            state <= random ? `THINKING : `EATING;
         end
         `HUNGRY : begin
            if (left != `EATING && right != `HUNGRY && right != `EATING) begin
               state <= `EATING;
            end
         end
      endcase
   end
end
endmodule
