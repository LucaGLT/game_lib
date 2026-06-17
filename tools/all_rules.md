# GRS Rule Graphs — turn_card_dungeon

## Base_Move

```mermaid
graph TD
    R_Base_Move["Base_Move\npriority = 100"]
    C_Base_Move_C_CanBaseMove{{"C_CanBaseMove"}}
    R_Base_Move --> C_Base_Move_C_CanBaseMove
    subgraph AG_Base_Move_AND_1[" AND "]
        direction LR
        C_Base_Move_C_HeroCanAct{{"C_HeroCanAct"}}
        C_Base_Move_C_DestinationValid{{"C_DestinationValid"}}
    end
    C_Base_Move_C_CanBaseMove -- AND --> AG_Base_Move_AND_1
    E_Base_Move_0["E_MoveHero\nMOVE_ACTOR"]
    AG_Base_Move_AND_1 --> E_Base_Move_0
    E_Base_Move_1["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Base_Move_0 --> E_Base_Move_1
    E_Base_Move_2["E_MoveLog\nMANUAL_EFFECT · opt"]
    E_Base_Move_1 --> E_Base_Move_2
    TGT_Base_Move(("Target_Hero\nSELECTED_ACTOR"))
    E_Base_Move_2 --> TGT_Base_Move
    ST_Base_Move_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Base_Move_1 -.-> ST_Base_Move_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Base_Move -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Base_Move_2 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Base_Move fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Base_Move_C_CanBaseMove fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Move_C_HeroCanAct fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Move_C_DestinationValid fill:#FFF3CD,stroke:#FFAA00
    style AG_Base_Move_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Base_Move_0 fill:#C8E6C9,stroke:#388E3C
    style E_Base_Move_1 fill:#C8E6C9,stroke:#388E3C
    style E_Base_Move_2 fill:#C8E6C9,stroke:#388E3C
    style TGT_Base_Move fill:#E1BEE7,stroke:#7B1FA2
    style ST_Base_Move_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Base_Attack

```mermaid
graph TD
    R_Base_Attack["Base_Attack\npriority = 110"]
    C_Base_Attack_C_CanBaseAttack{{"C_CanBaseAttack"}}
    R_Base_Attack --> C_Base_Attack_C_CanBaseAttack
    subgraph AG_Base_Attack_AND_1[" AND "]
        direction LR
        C_Base_Attack_C_HeroCanAct{{"C_HeroCanAct"}}
        C_Base_Attack_C_EnemyExists{{"C_EnemyExists"}}
        C_Base_Attack_C_EnemyAdjacent{{"C_EnemyAdjacent"}}
        C_Base_Attack_C_EnemyAlive{{"C_EnemyAlive"}}
    end
    C_Base_Attack_C_CanBaseAttack -- AND --> AG_Base_Attack_AND_1
    E_Base_Attack_0["E_BaseAttackDmg\nDEAL_DAMAGE"]
    AG_Base_Attack_AND_1 --> E_Base_Attack_0
    E_Base_Attack_1["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Base_Attack_0 --> E_Base_Attack_1
    E_Base_Attack_2["E_AttackLog\nMANUAL_EFFECT · opt"]
    E_Base_Attack_1 --> E_Base_Attack_2
    TGT_Base_Attack(("Target_Hero\nSELECTED_ACTOR"))
    E_Base_Attack_2 --> TGT_Base_Attack
    ST_Base_Attack_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Base_Attack_1 -.-> ST_Base_Attack_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Base_Attack -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Base_Attack_2 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Base_Attack fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Base_Attack_C_CanBaseAttack fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Attack_C_HeroCanAct fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Attack_C_EnemyExists fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Attack_C_EnemyAdjacent fill:#FFF3CD,stroke:#FFAA00
    style C_Base_Attack_C_EnemyAlive fill:#FFF3CD,stroke:#FFAA00
    style AG_Base_Attack_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Base_Attack_0 fill:#C8E6C9,stroke:#388E3C
    style E_Base_Attack_1 fill:#C8E6C9,stroke:#388E3C
    style E_Base_Attack_2 fill:#C8E6C9,stroke:#388E3C
    style TGT_Base_Attack fill:#E1BEE7,stroke:#7B1FA2
    style ST_Base_Attack_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_Fireball

```mermaid
graph TD
    R_Card_Fireball["Card_Fireball\npriority = 200"]
    subgraph AG_Card_Fireball_AND_1[" AND "]
        direction LR
        C_Card_Fireball_C_CanPlayCard{{"C_CanPlayCard"}}
        C_Card_Fireball_CARD_IN_ZONE{{"CARD_IN_ZONE\nhand"}}
    end
    R_Card_Fireball -- AND --> AG_Card_Fireball_AND_1
    E_Card_Fireball_0["E_FireballDamage\nDEAL_DAMAGE"]
    AG_Card_Fireball_AND_1 --> E_Card_Fireball_0
    E_Card_Fireball_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_Fireball_0 --> E_Card_Fireball_1
    E_Card_Fireball_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_Fireball_1 --> E_Card_Fireball_2
    E_Card_Fireball_3["E_FireballLog\nMANUAL_EFFECT · opt"]
    E_Card_Fireball_2 --> E_Card_Fireball_3
    TGT_Card_Fireball(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_Fireball_3 --> TGT_Card_Fireball
    ST_Card_Fireball_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_Fireball_2 -.-> ST_Card_Fireball_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_Fireball -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_Fireball_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_Fireball fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_Fireball_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Fireball_CARD_IN_ZONE fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_Fireball_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_Fireball_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Fireball_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Fireball_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Fireball_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_Fireball fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_Fireball_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_Shield

```mermaid
graph TD
    R_Card_Shield["Card_Shield\npriority = 200"]
    C_Card_Shield_C_CanPlayCard{{"C_CanPlayCard"}}
    R_Card_Shield --> C_Card_Shield_C_CanPlayCard
    subgraph AG_Card_Shield_AND_1[" AND "]
        direction LR
        C_Card_Shield_C_HeroCanAct{{"C_HeroCanAct"}}
        C_Card_Shield_C_CardInHand{{"C_CardInHand"}}
    end
    C_Card_Shield_C_CanPlayCard -- AND --> AG_Card_Shield_AND_1
    E_Card_Shield_0["E_ShieldApply\nAPPLY_STATUS"]
    AG_Card_Shield_AND_1 --> E_Card_Shield_0
    E_Card_Shield_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_Shield_0 --> E_Card_Shield_1
    E_Card_Shield_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_Shield_1 --> E_Card_Shield_2
    E_Card_Shield_3["E_ShieldLog\nMANUAL_EFFECT · opt"]
    E_Card_Shield_2 --> E_Card_Shield_3
    TGT_Card_Shield(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_Shield_3 --> TGT_Card_Shield
    ST_Card_Shield_defended["defended\nREFRESH · UNTIL_NEXT_TURN"]
    E_Card_Shield_0 -.-> ST_Card_Shield_defended
    ST_Card_Shield_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_Shield_2 -.-> ST_Card_Shield_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_Shield -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_Shield_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_Shield fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_Shield_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Shield_C_HeroCanAct fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Shield_C_CardInHand fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_Shield_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_Shield_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Shield_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Shield_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Shield_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_Shield fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_Shield_defended fill:#FFE0B2,stroke:#E65100
    style ST_Card_Shield_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_Cure

```mermaid
graph TD
    R_Card_Cure["Card_Cure\npriority = 200"]
    C_Card_Cure_C_CanPlayCard{{"C_CanPlayCard"}}
    R_Card_Cure --> C_Card_Cure_C_CanPlayCard
    subgraph AG_Card_Cure_AND_1[" AND "]
        direction LR
        C_Card_Cure_C_HeroCanAct{{"C_HeroCanAct"}}
        C_Card_Cure_C_CardInHand{{"C_CardInHand"}}
    end
    C_Card_Cure_C_CanPlayCard -- AND --> AG_Card_Cure_AND_1
    E_Card_Cure_0["E_CureHeal\nHEAL"]
    AG_Card_Cure_AND_1 --> E_Card_Cure_0
    E_Card_Cure_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_Cure_0 --> E_Card_Cure_1
    E_Card_Cure_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_Cure_1 --> E_Card_Cure_2
    E_Card_Cure_3["E_CureLog\nMANUAL_EFFECT · opt"]
    E_Card_Cure_2 --> E_Card_Cure_3
    TGT_Card_Cure(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_Cure_3 --> TGT_Card_Cure
    ST_Card_Cure_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_Cure_2 -.-> ST_Card_Cure_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_Cure -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_Cure_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_Cure fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_Cure_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Cure_C_HeroCanAct fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Cure_C_CardInHand fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_Cure_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_Cure_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Cure_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Cure_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Cure_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_Cure fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_Cure_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_QuickStrike

```mermaid
graph TD
    R_Card_QuickStrike["Card_QuickStrike\npriority = 200"]
    subgraph AG_Card_QuickStrike_AND_1[" AND "]
        direction LR
        C_Card_QuickStrike_C_CanPlayCard{{"C_CanPlayCard"}}
        C_Card_QuickStrike_C_EnemyVisible{{"C_EnemyVisible"}}
    end
    R_Card_QuickStrike -- AND --> AG_Card_QuickStrike_AND_1
    E_Card_QuickStrike_0["E_QuickStrikeDmg\nDEAL_DAMAGE"]
    AG_Card_QuickStrike_AND_1 --> E_Card_QuickStrike_0
    E_Card_QuickStrike_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_QuickStrike_0 --> E_Card_QuickStrike_1
    E_Card_QuickStrike_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_QuickStrike_1 --> E_Card_QuickStrike_2
    E_Card_QuickStrike_3["E_QuickStrikeLog\nMANUAL_EFFECT · opt"]
    E_Card_QuickStrike_2 --> E_Card_QuickStrike_3
    TGT_Card_QuickStrike(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_QuickStrike_3 --> TGT_Card_QuickStrike
    ST_Card_QuickStrike_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_QuickStrike_2 -.-> ST_Card_QuickStrike_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_QuickStrike -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_QuickStrike_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_QuickStrike fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_QuickStrike_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_QuickStrike_C_EnemyVisible fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_QuickStrike_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_QuickStrike_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_QuickStrike_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_QuickStrike_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_QuickStrike_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_QuickStrike fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_QuickStrike_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_ArcaneBolt

```mermaid
graph TD
    R_Card_ArcaneBolt["Card_ArcaneBolt\npriority = 200"]
    subgraph AG_Card_ArcaneBolt_AND_1[" AND "]
        direction LR
        C_Card_ArcaneBolt_C_CanPlayCard{{"C_CanPlayCard"}}
        C_Card_ArcaneBolt_C_EnemyVisible{{"C_EnemyVisible"}}
        C_Card_ArcaneBolt_C_EnoughMana{{"C_EnoughMana"}}
    end
    R_Card_ArcaneBolt -- AND --> AG_Card_ArcaneBolt_AND_1
    E_Card_ArcaneBolt_0["E_ArcaneBoltDmg\nDEAL_DAMAGE"]
    AG_Card_ArcaneBolt_AND_1 --> E_Card_ArcaneBolt_0
    E_Card_ArcaneBolt_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_ArcaneBolt_0 --> E_Card_ArcaneBolt_1
    E_Card_ArcaneBolt_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_ArcaneBolt_1 --> E_Card_ArcaneBolt_2
    E_Card_ArcaneBolt_3["E_ArcaneBoltLog\nMANUAL_EFFECT · opt"]
    E_Card_ArcaneBolt_2 --> E_Card_ArcaneBolt_3
    TGT_Card_ArcaneBolt(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_ArcaneBolt_3 --> TGT_Card_ArcaneBolt
    ST_Card_ArcaneBolt_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_ArcaneBolt_2 -.-> ST_Card_ArcaneBolt_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_ArcaneBolt -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_ArcaneBolt_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_ArcaneBolt fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_ArcaneBolt_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_ArcaneBolt_C_EnemyVisible fill:#FFF3CD,stroke:#FFAA00
    style C_Card_ArcaneBolt_C_EnoughMana fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_ArcaneBolt_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_ArcaneBolt_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_ArcaneBolt_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_ArcaneBolt_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_ArcaneBolt_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_ArcaneBolt fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_ArcaneBolt_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```


---

## Card_Rally

```mermaid
graph TD
    R_Card_Rally["Card_Rally\npriority = 200"]
    subgraph AG_Card_Rally_AND_1[" AND "]
        direction LR
        C_Card_Rally_C_CanPlayCard{{"C_CanPlayCard"}}
        C_Card_Rally_C_AllyInRoom{{"C_AllyInRoom"}}
    end
    R_Card_Rally -- AND --> AG_Card_Rally_AND_1
    E_Card_Rally_0["E_RallyHeal\nHEAL"]
    AG_Card_Rally_AND_1 --> E_Card_Rally_0
    E_Card_Rally_1["E_DiscardPlayedCard\nMOVE_CARD_TO_ZONE · opt"]
    E_Card_Rally_0 --> E_Card_Rally_1
    E_Card_Rally_2["E_MarkActionUsed\nAPPLY_STATUS"]
    E_Card_Rally_1 --> E_Card_Rally_2
    E_Card_Rally_3["E_RallyLog\nMANUAL_EFFECT · opt"]
    E_Card_Rally_2 --> E_Card_Rally_3
    TGT_Card_Rally(("Target_Hero\nSELECTED_ACTOR"))
    E_Card_Rally_3 --> TGT_Card_Rally
    ST_Card_Rally_action_used["action_used\nONE_ONLY · UNTIL_NEXT_TURN"]
    E_Card_Rally_2 -.-> ST_Card_Rally_action_used
    TR_T_BlockDoubleAction["T_BlockDoubleAction\npriority = 5"]
    EV_T_BlockDoubleAction{"ON_EVENT\nACTION_SUBMITTED"}
    TR_T_BlockDoubleAction --> EV_T_BlockDoubleAction
    R_Card_Rally -.-> EV_T_BlockDoubleAction
    TC_T_BlockDoubleAction{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_BlockDoubleAction --> TC_T_BlockDoubleAction
    TE_T_BlockDoubleAction_0["E_ActionBlocked\nMANUAL_EFFECT · opt"]
    TC_T_BlockDoubleAction --> TE_T_BlockDoubleAction_0
    TR_T_TurnEndNotify["T_TurnEndNotify\npriority = 999"]
    EV_T_TurnEndNotify{"ON_EVENT\nACTION_COMPLETED"}
    TR_T_TurnEndNotify --> EV_T_TurnEndNotify
    E_Card_Rally_3 -.-> EV_T_TurnEndNotify
    TC_T_TurnEndNotify{{"ACTOR_HAS_STATUS\naction_used"}}
    EV_T_TurnEndNotify --> TC_T_TurnEndNotify
    TE_T_TurnEndNotify_0["E_TurnEndLog\nMANUAL_EFFECT · opt"]
    TC_T_TurnEndNotify --> TE_T_TurnEndNotify_0

    style R_Card_Rally fill:#CCE5FF,stroke:#3399FF,stroke-width:2px
    style C_Card_Rally_C_CanPlayCard fill:#FFF3CD,stroke:#FFAA00
    style C_Card_Rally_C_AllyInRoom fill:#FFF3CD,stroke:#FFAA00
    style AG_Card_Rally_AND_1 fill:#FFFDE7,stroke:#F9A825
    style E_Card_Rally_0 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Rally_1 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Rally_2 fill:#C8E6C9,stroke:#388E3C
    style E_Card_Rally_3 fill:#C8E6C9,stroke:#388E3C
    style TGT_Card_Rally fill:#E1BEE7,stroke:#7B1FA2
    style ST_Card_Rally_action_used fill:#FFE0B2,stroke:#E65100
    style TR_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style EV_T_BlockDoubleAction fill:#FFCDD2,stroke:#C62828
    style TC_T_BlockDoubleAction fill:#FFF3CD,stroke:#FFAA00
    style TE_T_BlockDoubleAction_0 fill:#C8E6C9,stroke:#388E3C
    style TR_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style EV_T_TurnEndNotify fill:#FFCDD2,stroke:#C62828
    style TC_T_TurnEndNotify fill:#FFF3CD,stroke:#FFAA00
    style TE_T_TurnEndNotify_0 fill:#C8E6C9,stroke:#388E3C
```
