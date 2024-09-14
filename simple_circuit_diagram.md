graph TD
    USB[USB 3.0 Port] -->|5V| LR[Linear Regulator]
    USB -->|5V| NP[NeoPixel]
    LR -->|3.3V| ESP[ESP01]
    
    C1[104 Capacitor] -->|Input| LR
    C2[100uF Capacitor] -->|Output| LR
    
    BC[Bulk Capacitor] -->|Optional| NP

    subgraph Power Supply
        USB
        LR
        C1
        C2
        BC
    end

    subgraph Microcontroller
        ESP
    end

    subgraph LED Strip
        NP
    end

    classDef component fill:#f9f,stroke:#333,stroke-width:2px;
    class USB,LR,ESP,NP component;
    classDef capacitor fill:#bfb,stroke:#333,stroke-width:2px;
    class C1,C2,BC capacitor;