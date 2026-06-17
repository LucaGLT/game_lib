# GRS Rule Graphs — dungeon_crawler

## Move_Hero

```mermaid
graph TD
    R_Move_Hero["Move_Hero\npriority = 100"]
    C_Move_Hero_C_CanMove{{"C_CanMove"}}
    R_Move_Hero --> C_Move_Hero_C_CanMove
    subgraph AG_Move_Hero_AND_1[" AND "]
        direction LR
        C_Move_Hero_C_HeroExists{{"C_HeroExists"}}
        C_Move_Hero_C_DestinationValid{{"C_DestinationValid"}}
    end
    C_Move_Hero_C_CanMove -- AND --> AG_Move_Hero_AND_1
    E_Move_Hero_0["E_MoveHero\nMOVE_ACTOR"]
    AG_Move_Hero_AND_1 --> E_Move_Hero_0
    E_Move_Hero_1["E_MoveLog\nMANUAL_EFFECT · opt"]
    E_Move_Hero_0 --> E_Move_Hero_1
    TGT_Move_Hero(("Target_Hero\nSELECTED_ACTOR"))
    E_Move_Hero_1 --> TGT_Move_Hero
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Move_Hero -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Move_Hero fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Move_Hero_C_CanMove fill:#FFF3CD,stroke:#FFAA00
    style C_Move_Hero_C_HeroExists fill:#FFF3CD,stroke:#FFAA00
    style C_Move_Hero_C_DestinationValid fill:#FFF3CD,stroke:#FFAA00
    style AG_Move_Hero_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Move_Hero_0 fill:#C8E6C9,stroke:#388E3C
    style E_Move_Hero_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Move_Hero fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Attack_Barehands

```mermaid
graph TD
    R_Attack_Barehands["Attack_Barehands\npriority = 200"]
    subgraph AG_Attack_Barehands_AND_1[" AND "]
        direction LR
        C_Attack_Barehands_C_CanAttack{{"C_CanAttack"}}
        C_Attack_Barehands_expr_1{{"NOT C_HeroHasWeapon"}}
    end
    R_Attack_Barehands -- AND --> AG_Attack_Barehands_AND_1
    E_Attack_Barehands_0["E_AttackDamage\nDEAL_DAMAGE"]
    AG_Attack_Barehands_AND_1 --> E_Attack_Barehands_0
    E_Attack_Barehands_1["E_AttackLog\nEMIT_EVENT"]
    E_Attack_Barehands_0 --> E_Attack_Barehands_1
    TGT_Attack_Barehands(("Target_Hero\nSELECTED_ACTOR"))
    E_Attack_Barehands_1 --> TGT_Attack_Barehands
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Attack_Barehands -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Attack_Barehands fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Attack_Barehands_C_CanAttack fill:#FFF3CD,stroke:#FFAA00
    style C_Attack_Barehands_expr_1 fill:#FFF3CD,stroke:#FFAA00
    style AG_Attack_Barehands_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Attack_Barehands_0 fill:#C8E6C9,stroke:#388E3C
    style E_Attack_Barehands_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Attack_Barehands fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Attack_WithSword

```mermaid
graph TD
    R_Attack_WithSword["Attack_WithSword\npriority = 210"]
    subgraph AG_Attack_WithSword_AND_1[" AND "]
        direction LR
        C_Attack_WithSword_C_CanAttack{{"C_CanAttack"}}
        C_Attack_WithSword_ACTOR_HAS_TAG{{"ACTOR_HAS_TAG\nequipped_weapon"}}
    end
    R_Attack_WithSword -- AND --> AG_Attack_WithSword_AND_1
    E_Attack_WithSword_0["E_AttackDamageWithSword\nDEAL_DAMAGE"]
    AG_Attack_WithSword_AND_1 --> E_Attack_WithSword_0
    E_Attack_WithSword_1["E_AttackLog\nEMIT_EVENT"]
    E_Attack_WithSword_0 --> E_Attack_WithSword_1
    TGT_Attack_WithSword(("Target_Hero\nSELECTED_ACTOR"))
    E_Attack_WithSword_1 --> TGT_Attack_WithSword
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Attack_WithSword -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Attack_WithSword fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Attack_WithSword_C_CanAttack fill:#FFF3CD,stroke:#FFAA00
    style C_Attack_WithSword_ACTOR_HAS_TAG fill:#FFF3CD,stroke:#FFAA00
    style AG_Attack_WithSword_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Attack_WithSword_0 fill:#C8E6C9,stroke:#388E3C
    style E_Attack_WithSword_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Attack_WithSword fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Defend_Stance

```mermaid
graph TD
    R_Defend_Stance["Defend_Stance\npriority = 150"]
    subgraph AG_Defend_Stance_AND_1[" AND "]
        direction LR
        C_Defend_Stance_C_HeroExists{{"C_HeroExists"}}
        C_Defend_Stance_C_TargetIsAlive{{"C_TargetIsAlive"}}
    end
    R_Defend_Stance -- AND --> AG_Defend_Stance_AND_1
    E_Defend_Stance_0["E_DefendStatus\nAPPLY_STATUS"]
    AG_Defend_Stance_AND_1 --> E_Defend_Stance_0
    E_Defend_Stance_1["E_ReduceDamage\nMANUAL_EFFECT · opt"]
    E_Defend_Stance_0 --> E_Defend_Stance_1
    TGT_Defend_Stance(("Target_Hero\nSELECTED_ACTOR"))
    E_Defend_Stance_1 --> TGT_Defend_Stance
    ST_Defend_Stance_defended["defended\nREFRESH · UNTIL_NEXT_TURN"]
    E_Defend_Stance_0 -.-> ST_Defend_Stance_defended
    TR_T_CleanupDefended["T_CleanupDefended\npriority = 100"]
    EV_T_CleanupDefended{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_CleanupDefended --> EV_T_CleanupDefended
    E_Defend_Stance_1 -.-> EV_T_CleanupDefended
    TC_T_CleanupDefended{{"ACTOR_HAS_STATUS\ndefended"}}
    EV_T_CleanupDefended --> TC_T_CleanupDefended
    TE_T_CleanupDefended_0["REMOVE_STATUS · opt"]
    TC_T_CleanupDefended --> TE_T_CleanupDefended_0
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Defend_Stance -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Defend_Stance fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Defend_Stance_C_HeroExists fill:#FFF3CD,stroke:#FFAA00
    style C_Defend_Stance_C_TargetIsAlive fill:#FFF3CD,stroke:#FFAA00
    style AG_Defend_Stance_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Defend_Stance_0 fill:#C8E6C9,stroke:#388E3C
    style E_Defend_Stance_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Defend_Stance fill:#E1BEE7,stroke:#7B1FA2
    style ST_Defend_Stance_defended fill:#FFE0B2,stroke:#E65100
    style TR_T_CleanupDefended fill:#FFCDD2,stroke:#C62828
    style EV_T_CleanupDefended fill:#FFCDD2,stroke:#C62828
    style TC_T_CleanupDefended fill:#FFF3CD,stroke:#FFAA00
    style TE_T_CleanupDefended_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Heal_Self

```mermaid
graph TD
    R_Heal_Self["Heal_Self\npriority = 120"]
    C_Heal_Self_C_HeroCanHeal{{"C_HeroCanHeal"}}
    R_Heal_Self --> C_Heal_Self_C_HeroCanHeal
    E_Heal_Self_0["E_HealSmall\nHEAL"]
    C_Heal_Self_C_HeroCanHeal --> E_Heal_Self_0
    E_Heal_Self_1["E_RemovePotion\nREMOVE_TAG"]
    E_Heal_Self_0 --> E_Heal_Self_1
    E_Heal_Self_2["E_HealLog\nEMIT_EVENT · opt"]
    E_Heal_Self_1 --> E_Heal_Self_2
    TGT_Heal_Self(("Target_Hero\nSELECTED_ACTOR"))
    E_Heal_Self_2 --> TGT_Heal_Self
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Heal_Self -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Heal_Self fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Heal_Self_C_HeroCanHeal fill:#FFF3CD,stroke:#FFAA00
    style E_Heal_Self_0 fill:#C8E6C9,stroke:#388E3C
    style E_Heal_Self_1 fill:#C8E6C9,stroke:#388E3C
    style E_Heal_Self_2 fill:#C8E6C9,stroke:#388E3C
    style TGT_Heal_Self fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Heal_Adjacent

```mermaid
graph TD
    R_Heal_Adjacent["Heal_Adjacent\npriority = 125"]
    C_Heal_Adjacent_C_HeroCanHeal{{"C_HeroCanHeal"}}
    R_Heal_Adjacent --> C_Heal_Adjacent_C_HeroCanHeal
    E_Heal_Adjacent_0["E_HealAdjacent\nHEAL · opt"]
    C_Heal_Adjacent_C_HeroCanHeal --> E_Heal_Adjacent_0
    E_Heal_Adjacent_1["E_RemovePotion\nREMOVE_TAG"]
    E_Heal_Adjacent_0 --> E_Heal_Adjacent_1
    TGT_Heal_Adjacent(("Target_Hero\nSELECTED_ACTOR"))
    E_Heal_Adjacent_1 --> TGT_Heal_Adjacent
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Heal_Adjacent -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Heal_Adjacent fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Heal_Adjacent_C_HeroCanHeal fill:#FFF3CD,stroke:#FFAA00
    style E_Heal_Adjacent_0 fill:#C8E6C9,stroke:#388E3C
    style E_Heal_Adjacent_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Heal_Adjacent fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Equip_BigSword

```mermaid
graph TD
    R_Equip_BigSword["Equip_BigSword\npriority = 80"]
    subgraph AG_Equip_BigSword_AND_1[" AND "]
        direction LR
        C_Equip_BigSword_C_HasBigSword{{"C_HasBigSword"}}
        C_Equip_BigSword_C_NoWeaponEquipped{{"C_NoWeaponEquipped"}}
    end
    R_Equip_BigSword -- AND --> AG_Equip_BigSword_AND_1
    E_Equip_BigSword_0["E_EquipBigSword\nADD_TAG"]
    AG_Equip_BigSword_AND_1 --> E_Equip_BigSword_0
    E_Equip_BigSword_1["E_SetWeaponType\nMANUAL_EFFECT · opt"]
    E_Equip_BigSword_0 --> E_Equip_BigSword_1
    TGT_Equip_BigSword(("Target_Hero\nSELECTED_ACTOR"))
    E_Equip_BigSword_1 --> TGT_Equip_BigSword
    TR_T_BlockIfStunned["T_BlockIfStunned\npriority = 50"]
    EV_T_BlockIfStunned{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockIfStunned --> EV_T_BlockIfStunned
    R_Equip_BigSword -.-> EV_T_BlockIfStunned
    TC_T_BlockIfStunned{{"ACTOR_HAS_TAG\nstunned"}}
    EV_T_BlockIfStunned --> TC_T_BlockIfStunned
    TE_T_BlockIfStunned_0["MANUAL_EFFECT · opt"]
    TC_T_BlockIfStunned --> TE_T_BlockIfStunned_0

    style R_Equip_BigSword fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Equip_BigSword_C_HasBigSword fill:#FFF3CD,stroke:#FFAA00
    style C_Equip_BigSword_C_NoWeaponEquipped fill:#FFF3CD,stroke:#FFAA00
    style AG_Equip_BigSword_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Equip_BigSword_0 fill:#C8E6C9,stroke:#388E3C
    style E_Equip_BigSword_1 fill:#C8E6C9,stroke:#388E3C
    style TGT_Equip_BigSword fill:#E1BEE7,stroke:#7B1FA2
    style TR_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockIfStunned fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockIfStunned fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockIfStunned_0 fill:#C8E6C9,stroke:#388E3C
```
