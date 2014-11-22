/* 
   Source   : traffic/traffic
   Synopsis : A sample traffic light design.
   Authors  : Cheng-Yin Wu, NTUGIEE
   Date     : 2013/02/20
   Version  : 1.0
*/

module trafficLight(
   // Property Output Ports
   p1,
   p2,
   p3,
   // General I/O Ports
   reset,
   clk,
   // Output Ports
   time_left
);

// Parameters
parameter RED    = 0;
parameter GREEN  = 1;
parameter YELLOW = 2;

// Property Output Ports
output         p1;
output         p2;
output         p3;
// General I/O Ports
input          reset;
input          clk;
// Output Ports
output [7:0]   time_left;

wire [5:0]     RED_count;
wire [5:0]     GREEN_count;
wire [2:0]     YELLOW_count;

reg  [1:0]     light;
reg  [7:0]     counter;

// Property Logics
assign p1 = (light != RED) && (light != GREEN) && (light != YELLOW);
assign p2 = ((light == RED) && (counter > RED_count)) || 
            ((light == GREEN) && (counter > GREEN_count)) || 
            ((light == YELLOW) && (counter > YELLOW_count));
assign p3 = light == YELLOW;

// Combinational Assignments
assign RED_count    = 6'd60;
assign GREEN_count  = 6'd40;
assign YELLOW_count = 3'd5;
assign time_left    = counter;

always @(posedge clk) begin
   if (!reset) begin
      light   <= RED;
      counter <= 8'd0;
   end
   else begin
      case (light)
         RED     : light <= (counter == 8'd0) ? GREEN  : RED;
         GREEN   : light <= (counter == 8'd0) ? YELLOW : GREEN;
         default : light <= (counter == 8'd0) ? RED    : YELLOW;
      endcase
      case (light)
         RED     : counter <= (counter == 8'd0) ? GREEN_count  : counter - 8'd1;
         GREEN   : counter <= (counter == 8'd0) ? YELLOW_count : counter - 8'd1;
         default : counter <= (counter == 8'd0) ? RED_count    : counter - 8'd1;
      endcase
   end
end

endmodule
