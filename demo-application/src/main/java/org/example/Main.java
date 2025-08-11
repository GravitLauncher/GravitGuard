package org.example;

//TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {
    public static void main(String[] args) throws Exception {
        for(int i=0;i<200;++i) {
            System.out.printf("Passed %d seconds\n", i);
            Thread.sleep(1000);
        }
    }
}