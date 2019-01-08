using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class UIScript : MonoBehaviour {

    public ServerComm srv_comm;
    public Button attack_btn;
    public Button connect_btn;
    public Text status_txt;
    public Text attack_txt;

    private string attack_msg = "";
    // Use this for initialization
    void Start () {
        //GameObject attack_btn = GameObject.Find("ButtonAttack");
        //Button btn = attack_btn.GetComponent<Button>();
        attack_btn.onClick.AddListener(delegate () {
            this.OnAttackClick(attack_btn.gameObject);
        });
        connect_btn.onClick.AddListener(delegate () {
            this.OnConnectClick(connect_btn.gameObject);
        });

        srv_comm.attack_callback += OnAttackCallback;
    }
	
	// Update is called once per frame
	void Update () {
        status_txt.text = srv_comm.GetStateDesc();

        lock (attack_msg)
        {
            attack_txt.text = attack_msg;
        }
        ServerComm.ConnState conn_state = srv_comm.GetState();
        if (conn_state == ServerComm.ConnState.CONNECTED)
        {
            attack_btn.interactable = true;
            connect_btn.interactable = false;
        }
        else
        {
            if (conn_state == ServerComm.ConnState.CONNECTING)
            {
                connect_btn.interactable = false;
            }
            else
            {
                connect_btn.interactable = true;
            }
            attack_btn.interactable = false;
        }
    }

    private void Awake()
    {
        status_txt.text = "";
        attack_txt.text = "";
    }

    public void OnAttackClick(GameObject sender)
    {
        Debug.Log("attack clicked");

        GameObject gameObject = GameObject.Find("Skeleton");

        Animator anim = gameObject.GetComponent<Animator>();
        anim.SetTrigger("AttackTrigger");

        srv_comm.Attack();
    }

    public void OnConnectClick(GameObject sender)
    {
        Debug.Log("connect clicked");

        srv_comm.ConnectToServer();
        
    }

    void OnAttackCallback(int attack_id, long latency)
    {
        lock (attack_msg)
        {
            attack_msg = string.Format("attack: {0}, latency:{1}ms", attack_id, latency);
        }
    }
}
