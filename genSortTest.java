import java.math.*;
import java.util.*;
import java.io.*;


//Generate a sample of 2^args[0] numbers to be sorted. 
public class genSortTest{
  public static void main(String[] args){
    FileWriter writer;
    BufferedWriter bufWriter;
    try{
      Random ran = new Random();
      writer = new FileWriter(args[0] + "bit_sampleset.txt");
      bufWriter = new BufferedWriter(writer);
      int value;
      value = (int)Math.pow(2,Integer.parseInt(args[0]));
      bufWriter.write(value + " ");
      for(int i=0;i<Math.pow(2,Integer.parseInt(args[0]));i++){
        do{
          value = ran.nextInt();
        }
        while(value < 100000 && value > -100000);
        bufWriter.write(value + " ");
      }
      bufWriter.close();
      writer.close();
    }
    catch(IOException e){
      e.printStackTrace();
    }
  }
}
