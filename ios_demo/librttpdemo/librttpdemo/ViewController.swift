//
//  ViewController.swift
//  librttpdemo
//
//  Created by alice on 2018/10/20.
//  Copyright © 2018年 rtttech. All rights reserved.
//

import UIKit
class ViewController: UIViewController {
    @IBOutlet var textfieldServer : UITextField!
    @IBOutlet var textfieldRTTPPort : UITextField!
    @IBOutlet var textfieldTCPPort : UITextField!
    @IBOutlet var labelRTTPLatency : UILabel!
    @IBOutlet var labelTCPLatency : UILabel!
    @IBOutlet var buttonStart : UIButton!
    @IBOutlet var buttonStop : UIButton!
    
    @IBAction func onStartButtonClicked(sender:AnyObject) {
        let server = textfieldServer.text
        let rttpPort = Int32(textfieldRTTPPort.text!)
        let tcpPort = Int32(textfieldTCPPort.text!)
        startPing(server, rttpPort!, tcpPort!)
        
        buttonStart.isEnabled = false
        buttonStop.isEnabled = true
    }
    
    @IBAction func onStopButtonClicked(sender:AnyObject) {
        stopPing()
        buttonStart.isEnabled = true
        buttonStop.isEnabled = false
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        buttonStart.isEnabled = true
        buttonStop.isEnabled = false
        // Do any additional setup after loading the view, typically from a nib.
        let timer = Timer.scheduledTimer(timeInterval: 0.4, target: self, selector: #selector(self.update), userInfo: nil, repeats: true)
        //Looks for single or multiple taps.
        let tap: UITapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(ViewController.dismissKeyboard))
        
        //Uncomment the line below if you want the tap not not interfere and cancel other interactions.
        //tap.cancelsTouchesInView = false
        
        view.addGestureRecognizer(tap)
    }

    @objc func update() {
        labelRTTPLatency.text = String(getRttpLatency()/1000) + "ms";
        labelTCPLatency.text = String(getTcpLatency()/1000) + "ms"
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    @objc func dismissKeyboard() {
        //Causes the view (or one of its embedded text fields) to resign the first responder status.
        view.endEditing(true)
    }

}

